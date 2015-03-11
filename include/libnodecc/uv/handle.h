#ifndef nodecc_uv_handle_h
#define nodecc_uv_handle_h

#include <cmath>
#include <functional>

#include "../common.h"
#include "../loop.h"


namespace node {
namespace uv {

template<typename T>
class handle {
public:
	NODE_ADD_CALLBACK(public, close, void);

public:
	typedef handle handle_type;


	explicit handle() {
		this->_handle.loop = nullptr;
		this->_handle.data = this;
	}

	virtual ~handle() {}

	handle(const handle&) = delete;
	handle& operator=(const handle&) = delete;


	operator node::loop&() { return reinterpret_cast<node::loop&>(*this->_handle.loop->data); }
	operator uv_loop_t*() { return this->_handle.loop; }
	operator uv_handle_t*() { return reinterpret_cast<uv_handle_t*>(&this->_handle); }

	operator node::loop&() const { return (node::loop&)(*this->_handle.loop->data); }
	operator uv_loop_t*() const { return (uv_loop_t*)this->_handle.loop; }
	operator const uv_handle_t*() const { return reinterpret_cast<const uv_handle_t*>(&this->_handle); }

	// casts *this to any libuv handle type
	template<typename U = T, typename = typename std::enable_if<!std::is_same<U, uv_handle_t>::value>::type>
	operator T*() { return &this->_handle; }
	template<typename U = T, typename = typename std::enable_if<!std::is_same<U, uv_handle_t>::value>::type>
	operator const T*() const { return &this->_handle; }

	template<typename U, typename V>
	static U* to_node(V* handle) { return dynamic_cast<U*>(((uv::handle<T>*)handle->data)); }

	template<class T1, class T2>
	friend bool operator==(const uv::handle<T1>& lhs, const uv::handle<T2>& rhs) noexcept {
		return &lhs._handle == &rhs._handle;
	}

	template<class T1, class T2>
	friend bool operator!=(const uv::handle<T1>& lhs, const uv::handle<T2>& rhs) noexcept {
		return &lhs._handle != &rhs._handle;
	}


	bool is_closing() const {
		return uv_is_closing(*this);
	}

	void close() {
		if (this->_handle.loop && !this->is_closing()) {
			uv_close(*this, [](uv_handle_t* handle) {
				uv::handle<T>* self = reinterpret_cast<uv::handle<T>*>(handle->data);

				if (self && self->_on_close) {
					/*
					 * After emitting the close event the std::function
					 * owning the callback should be cleared.
					 * This will ensure that smart pointers are reset.
					 * But move the std::function to the stack just in case that
					 * the handle gets deleted within the close callback.
					 */
					on_close_t close = std::move(self->_on_close);
					close();
				}
			});
		}
	}

	template<typename F>
	void close(F&& f) {
		this->on_close(std::forward<F>(f));
		this->close();
	}

	void ref() {
		uv_ref(*this);
	}

	void unref() {
		uv_unref(*this);
	}

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
