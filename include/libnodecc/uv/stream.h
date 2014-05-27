#ifndef node_cpp_uv_stream_h
#define node_cpp_uv_stream_h

#include "../common.h"
#include "handle.h"


namespace uv {
	template<typename T>
	class stream : public uv::handle<T> {
	public:
		typedef std::function<void(size_t suggested_size, uv_buf_t *buf)> on_alloc_t;
		typedef std::function<void(ssize_t nread, const uv_buf_t *buf)> on_read_t;
		typedef std::function<void(int err)> on_write_t;


		bool read_start() {
			// TODO getter/setter
			if (!this->on_alloc || !this->on_read) {
				return false;
			}

			return 0 == uv_read_start(reinterpret_cast<uv_stream_t*>(this->uv_handle()), [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
				auto self = reinterpret_cast<uv::stream<T>*>(handle->data);

				if (self) {
					self->on_alloc(suggested_size, buf);
				}
			}, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
				auto self = reinterpret_cast<uv::stream<T>*>(stream->data);

				if (self) {
					self->on_read(nread, buf);
				}
			});
		}

		bool read_stop() {
			return 0 == uv_read_stop(reinterpret_cast<uv_stream_t*>(this->uv_handle()));
		}

		// TODO: std::move functions &&
		void write(const std::string &str) {
			packed_write_req *packed_req = new packed_write_req();
			packed_req->buf = str;
			this->write(packed_req);
		}

		void write(const std::string &str, const on_write_t &on_write) {
			packed_write_req *packed_req = new packed_write_req();
			packed_req->buf = str;
			packed_req->cb = on_write;
			this->write(packed_req);
		}


		on_alloc_t on_alloc;
		on_read_t  on_read;

	private:
		class packed_write_req {
		public:
			uv_write_t req;
			std::string buf;
			on_write_t cb;

			inline uv_buf_t uv_buf() { return uv_buf_init(const_cast<char *>(this->buf.data()), (unsigned int)this->buf.length()); }
		};

		void write(packed_write_req *packed_req) {
			uv_write(&packed_req->req, reinterpret_cast<uv_stream_t*>(this->uv_handle()), &packed_req->uv_buf(), 1, [](uv_write_t *req, int status) {
				packed_write_req *packed_req = container_of(req, packed_write_req, req);

				if (packed_req->cb) {
					packed_req->cb(status);
				}

				delete packed_req;
			});
		}
	};
}

#endif
