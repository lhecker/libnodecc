#ifndef nodecc_uv_handle_h
#define nodecc_uv_handle_h

#include <functional>
#include <cmath>

#include "loop.h"


namespace uv {

template<typename T>
class handle {
public:
	typedef handle handle_type;

	typedef std::function<void()> on_close_t;


	explicit handle() {
		this->_handle.data = this;
	}

	handle(const handle&) = delete;
	handle& operator=(const handle&) = delete;


	operator uv::loop&() { return reinterpret_cast<uv::loop&>(*this->_handle.loop); }
	operator uv_loop_t*() { return this->_handle.loop; }
	operator uv_handle_t*() { return reinterpret_cast<uv_handle_t*>(&this->_handle); }
	operator T*() { return &this->_handle; }

	operator const uv::loop&() const { return reinterpret_cast<const uv::loop&>(*this->_handle.loop); }
	operator const uv_loop_t*() const { return this->_handle.loop; }
	operator const uv_handle_t*() const { return reinterpret_cast<const uv_handle_t*>(&this->_handle); }
	operator const T*() const { return &this->_handle; }


	template<class T1, class T2>
	friend bool operator==(const uv::handle<T1>& lhs, const uv::handle<T2>& rhs) noexcept {
		return &lhs._handle == &rhs._handle;
	}

	template<class T1, class T2>
	friend bool operator!=(const uv::handle<T1>& lhs, const uv::handle<T2>& rhs) noexcept {
		return &lhs._handle != &rhs._handle;
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

/*
 * Instances are likely to be aligned along the size of the class.
 * Those least significant bits which "represent" that alignment are therefore
 * likely to be some static value (e.g. zero) and can be cut off for a better hash value.
 */
template<typename T>
struct std::hash<uv::handle<T>> {
	size_t operator()(const uv::handle<T> &val) const {
		static const size_t shift = static_cast<size_t>(std::log2(1 + sizeof(uv::handle<T>)));
		return size_t(static_cast<const uv_handle_t*>(val)) >> shift;
	}
};

template<typename T>
struct std::equal_to<uv::handle<T>> {
	bool operator()(const uv::handle<T> &left, const uv::handle<T> &right) const {
		return static_cast<uv_handle_t*>(left) == static_cast<uv_handle_t*>(right);
	}
};

#endif // nodecc_uv_handle_h
