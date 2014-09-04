#ifndef nodecc_uv_stream_h
#define nodecc_uv_stream_h

#include <cstddef>
#include <cstdlib>
#include <vector>

#include "../util/buffer.h"
#include "handle.h"


namespace uv {

template<typename T>
class stream : public uv::handle<T> {
public:
	typedef stream stream_type;

	typedef std::function<void(int err, const util::buffer &buffer)> on_read_t;
	typedef std::function<void(int err)> on_write_t;
	

	explicit stream() : uv::handle<T>() {}

	operator uv_stream_t*() {
		return reinterpret_cast<uv_stream_t*>(&this->_handle);
	}

	bool read_start() {
		// TODO getter/setter
		if (!this->on_read) {
			return false;
		}

		return 0 == uv_read_start(*this, [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = suggested_size;
		}, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
			auto self = reinterpret_cast<uv::stream<T>*>(stream->data);

			util::buffer buffer;

			if (nread >= 0 && buf->base) {
				buffer.reset(buf->base, nread, util::strong);
			}

			if (nread != 0 && self->on_read) {
				self->on_read(nread < 0 ? nread : 0, buffer);
			}
		});
	}

	bool read_stop() {
		return 0 == uv_read_stop(*this);
	}

	bool write(const util::buffer &buf, const on_write_t &on_write = nullptr) {
		return this->write(&buf, 1, on_write);
	}

	bool write(const util::buffer bufs[], size_t bufcnt, const on_write_t &on_write = nullptr) {
		auto packed_req = new packed_write_req(bufs, bufcnt, on_write);
		auto uv_bufs = static_cast<uv_buf_t*>(alloca(bufcnt * sizeof(uv_buf_t)));

		for (size_t i = 0; i < bufcnt; i++) {
			uv_bufs[i].base = bufs[i].data<char>();
			uv_bufs[i].len  = bufs[i].size();
		}

		return 0 == uv_write(&packed_req->req, *this, uv_bufs, bufcnt, [](uv_write_t *req, int status) {
			packed_write_req *packed_req = reinterpret_cast<packed_write_req*>(req);

			if (packed_req->cb) {
				packed_req->cb(status);
			}

			delete packed_req;
		});
	}


	on_read_t on_read;

private:
	struct packed_write_req {
		constexpr packed_write_req(const util::buffer bufs[], size_t bufcnt, const on_write_t &cb) : cb(cb), bufs(bufs, bufs + bufcnt) {
			static_assert(std::is_standard_layout<packed_write_req>::value && offsetof(packed_write_req, req) == 0, "packed_write_req needs to be in standard layout!");
		}

		uv_write_t req;
		on_write_t cb;
		std::vector<util::buffer> bufs;
	};
};

} // namespace uv

#endif // nodecc_uv_stream_h
