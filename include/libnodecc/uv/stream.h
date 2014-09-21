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
public:
	typedef std::function<void(int err, const node::buffer& buffer)> on_read_t;
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

			if (nread != 0 && self->on_read) {
				node::buffer buffer(buf->base, nread, node::strong);
				self->on_read(nread < 0 ? nread : 0, buffer);
			} else {
				free(buf->base);
			}
		});
	}

	bool read_stop() {
		return 0 == uv_read_stop(*this);
	}

	bool write(const node::buffer& buf, const on_write_t& on_write = nullptr) {
		return this->write(&buf, 1, on_write);
	}

	bool write(const node::buffer bufs[], size_t bufcnt, const on_write_t& on_write = nullptr) {
		struct packed_req {
			constexpr packed_req(const node::buffer bufs[], size_t bufcnt, const on_write_t& cb) : cb(cb), bufs(bufs, bufs + bufcnt) {}

			uv_write_t req;
			on_write_t cb;
			std::vector<node::buffer> bufs;
		};

		packed_req* pack = new packed_req(bufs, bufcnt, on_write);
		uv_buf_t* uv_bufs = static_cast<uv_buf_t*>(alloca(bufcnt * sizeof(uv_buf_t)));

		for (size_t i = 0; i < bufcnt; i++) {
			uv_bufs[i].base = bufs[i].data<char>();
			uv_bufs[i].len  = bufs[i].size();
		}

		return 0 == uv_write(&pack->req, *this, uv_bufs, bufcnt, [](uv_write_t* req, int status) {
			packed_req* pack = reinterpret_cast<packed_req*>(req);

			if (pack->cb) {
				pack->cb(status);
			}

			delete pack;
		});
	}

	void shutdown() {
		if (!uv_is_closing(*this)) {
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


	on_read_t on_read;
};

} // namespace uv
} // namespace node

#endif // nodecc_uv_stream_h
