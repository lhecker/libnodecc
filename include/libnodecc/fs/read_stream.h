#ifndef nodecc_fs_h
#define nodecc_fs_h

#include "../buffer.h"
#include "../stream.h"
#include "../uv/handle.h"

#include <fcntl.h>


namespace node {
namespace fs {


// x = fs.createReadStream('ToDo.txt'); emit = x.emit; x.emit = function() { console.log(arguments); emit.apply(x, arguments); }; x.resume();
class read_stream : public node::stream::readable<int, node::buffer> {
public:
	read_stream(node::loop& loop, const node::buffer_view& path, int mode) {
		node::mutable_buffer buf;
		buf.set_capacity(path.size() + 1);
		buf.append(path);
		buf.push_back('\0');

		const int r = uv_fs_open(loop, &this->_req, buf.data<char>(), O_RDONLY, 0, [](uv_fs_t* req) {
			auto self = static_cast<read_stream*>(req->data);

			if (req->result == 0) {
				self->_file = req->result;
				self->_is_reading = false;
				self->_read();
			} else {
				self->on_error.emit(req->result);
				self->on_end.emit();
			}

			uv_fs_req_cleanup(req);
		});
	}


	void resume() override {
		this->_flowing = true;
		this->_read();
	}

	void pause() override {
		this->_flowing = false;
	}

	void close() {
		if (this->_req.loop) {
			uv_fs_close(this->_req.loop, &this->_req, this->_file, [](uv_fs_t* req) {
				auto self = reinterpret_cast<read_stream*>(req->data);

				if (self && self->on_close) {
					// see uv::handle<>
					decltype(self->on_close) on_close;
					self->on_close.swap(on_close);

					on_close.emit();
				}
			});
		}
	}

	template<typename F>
	void close(F&& f) {
		this->on_close(std::forward<F>(f));
		this->close();
	}

	node::event<node::buffer()> on_close;
	node::event<node::buffer()> on_alloc;

private:
	void _read() {
		if (this->_flowing && !this->_is_reading) {
			const size_t suggested_size = UINT16_MAX;
			const auto r = this->on_alloc.emit();
			uv_buf_t buf;

			if (r) {
				this->_alloc_buffer = r.value();
			} else {
				this->_alloc_buffer.reset(suggested_size);
			}

			buf.base = this->_alloc_buffer.template data<char>();
			buf.len = this->_alloc_buffer.size();

			const int r = uv_fs_read(this->_req.loop, &this->_req, this->_file, &buf, 1, 0, [](uv_fs_t* req) {
				auto self = reinterpret_cast<read_stream*>(req->data);

				if (req->result > 0 && self->on_data) {
					const node::buffer buf = self->_alloc_buffer.slice(0, req->result);
					self->on_data.emit(&buf, 1);
				} else if (req->result < 0) {
					self->on_error.emit(int(req->result));
					self->on_end.emit();
				}

				self->_is_reading = false;
				self->_alloc_buffer.reset();

				self->_read();
			});

			if (r == 0) {
				this->_is_reading = true;
			} else {
				this->on_error.emit(r);
			}
		}
	}

	node::buffer _alloc_buffer;
	uv_fs_t _req;
	uv_file _file;
	bool _flowing = false;
	bool _is_reading = true;
	bool _should_close = false;
};

} // namespace fs
} // namespace node

#endif // nodecc_fs_event_h
