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
class stream : public node::uv::handle<T>, public node::stream::duplex<node::buffer, int> {
public:
	NODE_ADD_CALLBACK(public, alloc, node::buffer, size_t suggested_size)

public:
	typedef std::function<void(int err)> on_write_t;


	explicit stream() : node::uv::handle<T>() {
	}

	operator uv_stream_t*() {
		return reinterpret_cast<uv_stream_t*>(&this->_handle);
	}

	void resume() override {
		if (static_cast<const uv_stream_t*>(*this)->read_cb) {
			return;
		}

		uv_read_start(*this, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			node::uv::stream<T>* self = reinterpret_cast<node::uv::stream<T>*>(handle->data);

			if (self->has_alloc_callback()) {
				self->_alloc_buffer = self->emit_alloc(suggested_size);
			} else if (self->_alloc_buffer.use_count() != 1 || self->_alloc_buffer.size() != suggested_size) {
				self->_alloc_buffer.reset(suggested_size);
			}

			buf->base = static_cast<char*>(self->_alloc_buffer);
			buf->len = self->_alloc_buffer.size();
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			node::uv::stream<T>* self = reinterpret_cast<node::uv::stream<T>*>(stream->data);

			if (nread > 0 && self->has_data_callback()) {
				const node::buffer buf = self->_alloc_buffer.slice(0, nread);
				self->emit_data_s(0, &buf, 1);
			} else if (nread < 0) {
				self->emit_data_s(int(nread), nullptr, 0);
				self->emit_end_s();

				if (nread == UV_EOF) {
					// the other side shut down it's side (FIN) --> gracefully shutdown
					self->end();
				} else {
					// other error --> hard close
					self->close();
				}
			}

			self->_alloc_buffer.reset();
		});
	}

	void pause() override {
		uv_read_stop(*this);
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

			a->base = b->data<char>();
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
			explicit packed_req(uv::stream<T>& stream, const node::buffer bufs[], size_t bufcnt, size_t total) : bufs(bufs, bufs + bufcnt), total(total) {
				stream.increase_watermark(this->total);
				this->req.data = &stream;
			}

			~packed_req() {
				uv::stream<T>* stream = reinterpret_cast<uv::stream<T>*>(this->req.data);
				stream->decrease_watermark(this->total);
			}

			uv_write_t req;
			std::vector<node::buffer> bufs;
			size_t total;
		};

		packed_req* pack = new packed_req(*this, bufs, bufcnt, total);

		int ret = uv_write(&pack->req, *this, uv_bufs, static_cast<unsigned int>(bufcnt), [](uv_write_t* req, int status) {
			packed_req* pack = reinterpret_cast<packed_req*>(req);
			uv::stream<T>* self = reinterpret_cast<uv::stream<T>*>(req->data);

			if (status != 0) {
				self->close();
			}

			delete pack;
		});

		if (ret != 0) {
			delete pack;
		}
	}

	void _end(const node::buffer bufs[], size_t bufcnt) override {
		if (!this->is_closing()) {
			this->_write(bufs, bufcnt);

			uv_shutdown_t* req = new uv_shutdown_t;
			req->data = this;

			int ret = uv_shutdown(req, *this, [](uv_shutdown_t* req, int status) {
				node::uv::stream<T>* self = reinterpret_cast<node::uv::stream<T>*>(req->data);
				self->close();
				delete req;
			});

			if (ret != 0) {
				delete req;
			}
		}
	}

private:
	node::buffer _alloc_buffer;
};

} // namespace uv
} // namespace node

#endif // nodecc_uv_stream_h
