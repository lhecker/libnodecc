#ifndef nodecc_uv_stream_h
#define nodecc_uv_stream_h

#include <cstddef>
#include <cstdlib>
#include <vector>

#include "../buffer.h"
#include "../stream.h"
#include "handle.h"


namespace node {
namespace uv {

template<typename T>
class stream : public node::uv::handle<T>, public node::stream::duplex<stream<T>, node::buffer> {
public:
	operator uv_stream_t*() {
		return reinterpret_cast<uv_stream_t*>(&this->_handle);
	}

protected:
	typedef node::uv::handle<T> handle_type;
	typedef node::stream::duplex<stream<T>, node::buffer> stream_type;

	~stream() override = default;

	void _resume() override {
		node::uv::check(uv_read_start(*this, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			auto self = reinterpret_cast<node::uv::stream<T>*>(handle->data);

			try {
				self->_alloc_buffer = node::buffer(4000);
			} catch (...) {
				return;
			}

			buf->base = self->_alloc_buffer.template data<char>();
			buf->len = self->_alloc_buffer.size();
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			auto self = reinterpret_cast<node::uv::stream<T>*>(stream->data);

			if (nread > 0) {
				const node::buffer buf = self->_alloc_buffer.slice(0, nread);
				self->emit(stream_type::data_event, buf);
			} else if (nread < 0) {
				if (nread == UV_EOF) {
					// the other side shut down it's side (FIN)
					self->_set_reading_ended();

					if (self->writable_has_ended()) {
						self->destroy();
					} else {
						self->end();
					}
				} else {
					// other error --> hard close
					self->emit(handle_type::error_event, node::uv::to_error(int(nread)));
					self->destroy();
				}
			}

			self->_alloc_buffer.reset();
		}));
	}

	void _pause() override {
		node::uv::check(uv_read_stop(*this));
	}

	void _write(const node::buffer bufs[], size_t bufcnt) override {
		if (bufcnt == 0) {
			return;
		}

		uv_buf_t* uv_bufs = static_cast<uv_buf_t*>(alloca(bufcnt * sizeof(uv_buf_t)));
		size_t total = 0;
		size_t i;

		for (i = 0; i < bufcnt; i++) {
			uv_buf_t* a = uv_bufs + i;
			const node::buffer* b = bufs + i;

			a->base = (char*)b->data();
			a->len  = b->size();

			total += b->size();
		}

		const int wi = uv_try_write(*this, uv_bufs, static_cast<unsigned int>(bufcnt));
		size_t wu = size_t(wi);

		// if uv_try_write() sent all data
		if (wi > 0) {
			if (wu == total) {
				// everything was written
				return;
			}

			// check if for some reason wu contains an erroneous value just in case
			if (wu > total) {
				// -EINVAL
				return;
			}

			// count how many buffers have been fully written...
			for (i = 0; i < bufcnt; i++) {
				size_t len = uv_bufs[i].len;

				if (len > wu) {
					break;
				}

				wu -= len;
			}

			// ...remove them from the lists...
			bufs    += i;
			uv_bufs += i;
			bufcnt  -= i;

			// ...and remove the bytes not fully written from the (now) first buffer
			uv_bufs[0].base += wu;
			uv_bufs[0].len  -= wu;
		}

		struct packed_req {
			explicit packed_req(uv::stream<T>& stream, const node::buffer bufs[], size_t bufcnt, size_t total) : ref_list(bufs, bufs + bufcnt), total(total) {
				stream._increase_watermark(this->total);
				this->req.data = &stream;
			}

			~packed_req() {
				uv::stream<T>* stream = reinterpret_cast<uv::stream<T>*>(this->req.data);
				stream->_decrease_watermark(this->total);
			}

			uv_write_t req;
			std::vector<node::buffer> ref_list;
			size_t total;
		};

		auto pack = std::unique_ptr<packed_req>(new packed_req(*this, bufs, bufcnt, total));

		node::uv::check(uv_write(&pack->req, *this, uv_bufs, static_cast<unsigned int>(bufcnt), [](uv_write_t* req, int status) {
			uv::stream<T>* self = reinterpret_cast<uv::stream<T>*>(req->data);

			delete reinterpret_cast<packed_req*>(req);

			if (status != 0) {
				self->destroy();
			}

			self->release();
		}));

		this->retain();
		pack.release();
	}

	void _end(const node::buffer bufs[], size_t bufcnt) override {
		if (!this->is_closing()) {
			if (bufcnt) {
				this->_write(bufs, bufcnt);
			}

			auto req = std::unique_ptr<uv_shutdown_t>(new uv_shutdown_t);
			req->data = this;

			node::uv::check(uv_shutdown(req.get(), *this, [](uv_shutdown_t* req, int status) {
				node::uv::stream<T>* self = reinterpret_cast<node::uv::stream<T>*>(req->data);

				delete req;

				self->_set_writing_ended();

				if (status != 0 || self->readable_has_ended()) {
					self->destroy();
				}

				self->release();
			}));

			this->retain();
			req.release();
		}
	}

private:
	node::buffer _alloc_buffer;
};

} // namespace uv
} // namespace node

#endif // nodecc_uv_stream_h
