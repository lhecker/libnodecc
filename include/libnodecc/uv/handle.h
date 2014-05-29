#ifndef nodecc_uv_handle_h
#define nodecc_uv_handle_h

#include <functional>
#include <libuv/uv.h>


namespace uv {

template<typename T>
class handle {
public:
	typedef std::function<void()> on_close_t;

	explicit handle() : _handle(new T) {
		this->_handle->data = this;
	};

	virtual ~handle() {
		if (this->_handle) {
			// make sure the libuv structure does not reference back to this deleted instance
			this->_handle->data = nullptr;

			this->close();
		}
	}


	inline T *uv_handle() const {
		return this->_handle;
	}

	void close() {
		uv_handle_t *handle = reinterpret_cast<uv_handle_t*>(this->_handle);

		if (handle && !uv_is_closing(handle)) {
			uv_close(handle, [](uv_handle_t *handle) {
				auto self = reinterpret_cast<uv::handle<T>*>(handle->data);

				if (self) {
					// the instance might get deleted in the callback => do not modify self after the callback
					self->_handle = nullptr;

					if (self->on_close) {
						self->on_close();
					}
				}

				delete reinterpret_cast<T*>(handle);
			});
		}
	}

	void close(on_close_t &cb) {
		this->on_close = cb;
		this->close();
	}

	void ref() {
		uv_ref(reinterpret_cast<uv_handle_t*>(this->uv_handle()));
	}

	void unref() {
		uv_unref(reinterpret_cast<uv_handle_t*>(this->uv_handle()));
	}


	on_close_t on_close;


protected:
	T *_handle;
};

} // namespace uv

#endif // nodecc_uv_handle_h
