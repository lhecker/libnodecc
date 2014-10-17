#ifndef nodecc_uv_stream_h
#define nodecc_uv_stream_h

#include <cstddef>
#include <cstdlib>
#include <vector>

#include "../buffer.h"
#include "handle.h"


namespace node {
namespace uv {

template<typename T>
class stream : public node::uv::handle<T> {
	NODE_ADD_CALLBACK(read, void, int err, const node::buffer& buffer)

public:
	typedef std::function<void(int err)> on_write_t;


	explicit stream() : node::uv::handle<T>() {}

	operator uv_stream_t*() {
		return reinterpret_cast<uv_stream_t*>(&this->_handle);
	}

	bool read_start() {
		if (static_cast<const uv_stream_t*>(*this)->read_cb) {
			return false;
		}

		return 0 == uv_read_start(*this, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = suggested_size;
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			auto self = reinterpret_cast<node::uv::stream<T>*>(stream->data);
			node::buffer buffer;

			if (nread > 0 && self->has_read_callback()) {
				buffer.reset(buf->base, nread, node::strong);
				self->emit_read_s(0, buffer);
				return;
			}

			if (nread < 0) {
				self->emit_read_s(static_cast<int>(nread), buffer);
				self->on_read(nullptr);

				if (nread == UV_EOF) {
					self->shutdown();
				} else {
					self->close();
				}
			}

			free(buf->base);
		});
	}

	bool read_stop() {
		return 0 == uv_read_stop(*this);
	}

	bool write(const node::buffer& buf, on_write_t cb = nullptr) {
		return this->write(&buf, 1, std::forward<on_write_t>(cb));
	}

	bool write(const node::buffer bufs[], size_t bufcnt, on_write_t cb = nullptr) {
		struct packed_req {
			explicit packed_req(uv::stream<T>& stream, const node::buffer bufs[], size_t bufcnt, const on_write_t& cb) : cb(std::move(cb)), bufs(bufs, bufs + bufcnt) {
				this->req.data = &stream;
			}

			uv_write_t req;
			on_write_t cb;
			std::vector<node::buffer> bufs;
		};

		packed_req* pack = new packed_req(*this, bufs, bufcnt, cb);
		uv_buf_t* uv_bufs = static_cast<uv_buf_t*>(alloca(bufcnt * sizeof(uv_buf_t)));

		for (size_t i = 0; i < bufcnt; i++) {
			uv_bufs[i].base = bufs[i].data<char>();
			uv_bufs[i].len  = bufs[i].size();
		}

		return 0 == uv_write(&pack->req, *this, uv_bufs, static_cast<unsigned int>(bufcnt), [](uv_write_t* req, int status) {
			packed_req* pack = reinterpret_cast<packed_req*>(req);
			uv::stream<T>* self = reinterpret_cast<uv::stream<T>*>(req->data);

			if (pack->cb) {
				pack->cb(status);
			}

			if (status != 0) {
				self->close();
				self->on_read(nullptr);
			}

			delete pack;
		});
	}

	void shutdown() {
		if (!this->is_closing()) {
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
};

} // namespace uv
} // namespace node

#endif // nodecc_uv_stream_h
