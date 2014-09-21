#ifndef nodecc_uv_handle_h
#define nodecc_uv_handle_h

#include <cmath>
#include <functional>

#include "../loop.h"


namespace node {
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


	operator node::loop&() { return reinterpret_cast<node::loop&>(*this->_handle.loop->data); }
	operator uv_loop_t*() { return this->_handle.loop; }
	operator uv_handle_t*() { return reinterpret_cast<uv_handle_t*>(&this->_handle); }

	operator const node::loop&() const { return reinterpret_cast<const node::loop&>(*this->_handle.loop->data); }
	operator const uv_loop_t*() const { return this->_handle.loop; }
	operator const uv_handle_t*() const { return reinterpret_cast<const uv_handle_t*>(&this->_handle); }

	template<typename U = T, typename = typename std::enable_if<!std::is_same<U, uv_handle_t>::value>::type>
	operator T*() { return &this->_handle; }
	template<typename U = T, typename = typename std::enable_if<!std::is_same<U, uv_handle_t>::value>::type>
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
			uv_close(*this, [](uv_handle_t* handle) {
				uv::handle<T>* self = reinterpret_cast<uv::handle<T>*>(handle->data);

				if (self && self->on_close) {
					self->on_close();
				}
			});
		}
	}

	void close(const on_close_t& cb) {
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
} // namespace node

template<typename T>
struct std::hash<node::uv::handle<T>> {
	size_t operator()(const node::uv::handle<T>& val) const {
		std::size_t x = size_t(static_cast<const uv_handle_t*>(val));
		return x + (x >> 3);
	}
};

template<typename T>
struct std::equal_to<node::uv::handle<T>> {
	bool operator()(const node::uv::handle<T>& left, const node::uv::handle<T>& right) const {
		return static_cast<uv_handle_t*>(left) == static_cast<uv_handle_t*>(right);
	}
};

#endif // nodecc_uv_handle_h
