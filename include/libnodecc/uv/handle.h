#ifndef nodecc_uv_handle_h
#define nodecc_uv_handle_h

#include "../callback.h"
#include "../loop.h"
#include "../signal.h"


namespace node {
namespace uv {

template<typename T>
class handle {
public:
	typedef handle handle_type;


	explicit handle() {
		this->_handle.loop = nullptr;
		this->_handle.data = this;
	}

	virtual ~handle() {}

	handle(const handle&) = delete;
	handle& operator=(const handle&) = delete;


	operator node::loop&() { return *static_cast<node::loop*>(this->_handle.loop->data); }
	operator uv_loop_t*() { return this->_handle.loop; }
	operator uv_handle_t*() { return reinterpret_cast<uv_handle_t*>(&this->_handle); }

	operator node::loop&() const { return *static_cast<node::loop*>(this->_handle.loop->data); }
	operator uv_loop_t*() const { return this->_handle.loop; }
	operator const uv_handle_t*() const { return reinterpret_cast<const uv_handle_t*>(&this->_handle); }

	/*
	 * SFINAE only works if the inner template depends on the outer template.
	 * --> <typename U = T, (subsequent usage of U instead of T)>
	 */
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
		return uv_is_closing(*this) != 0;
	}

	bool is_active() const {
		return uv_is_active(*this) != 0;
	}

	void destroy() {
		if (this->_handle.loop && !this->is_closing()) {
			uv_close(*this, [](uv_handle_t* handle) {
				auto self = reinterpret_cast<uv::handle<T>*>(handle->data);

				if (self) {
					self->_destroy();
					self->close_signal.emit_and_clear();
				}
			});
		}
	}

	void ref() {
		uv_ref(*this);
	}

	void unref() {
		uv_unref(*this);
	}


	node::signal<void()> close_signal;

protected:
	virtual void _destroy() {
	}

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
