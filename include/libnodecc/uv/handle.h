#ifndef nodecc_uv_handle_h
#define nodecc_uv_handle_h

#include <functional>
#include <uv.h>


namespace uv {

template<typename T>
class handle {
public:
	typedef std::function<void()> on_close_t;

	explicit handle() {
		this->_handle.data = this;
	}

	operator uv_loop_t*() {
		return this->_handle.loop;
	}

	operator uv_handle_t*() {
		return reinterpret_cast<uv_handle_t*>(&this->_handle);
	}

	operator T*() {
		return &this->_handle;
	}

	void close() {
		if (!uv_is_closing(*this)) {
			uv_close(*this, [](uv_handle_t *handle) {
				auto self = reinterpret_cast<uv::handle<T>*>(handle->data);

				if (self && self->on_close) {
					self->on_close();
				}
			});
		}
	}

	void close(on_close_t &cb) {
		this->on_close = cb;
		this->close();
	}

	void ref() {
		uv_ref(*this);
	}

	void unref() {
		uv_unref(*this);
	}


	on_close_t on_close;


protected:
	T _handle;
};

} // namespace uv

#endif // nodecc_uv_handle_h
