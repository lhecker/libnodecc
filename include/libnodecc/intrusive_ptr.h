#ifndef nodecc_object_h
#define nodecc_object_h

#include <cassert>
#include <type_traits>

#include "callback.h"
#include "signal.h"


namespace node {

template<typename T>
class shared_ptr;

/*
 * Reference counting base class for libnodecc.
 *
 * Due to the asynchronous programming model reference counting is needed
 * to comfortably ensure that an "object" will only get deleted if
 * it's currently not running an asynchronous callback etc.
 *
 * @attention Subclasses SHOULD make their destructor protected to enforce the usage of destroy() etc.
 */
class intrusive_ptr {
	template<typename T>
	friend class shared_ptr;

private:
	intrusive_ptr* _responsible_object = nullptr;
	unsigned int   _use_count          = 1;
	bool           _is_destroyed       = false;
	bool           _is_shared          = false;

public:
	struct destructor {
		void operator()(intrusive_ptr* o) {
			o->destroy();
		}
	};

	intrusive_ptr() = default;
	intrusive_ptr(intrusive_ptr&&) = delete;

	decltype(_use_count) use_count() const noexcept {
		return this->_use_count;
	}

	template<typename S>
	shared_ptr<S> shared_from_this() {
		return shared_ptr<S>(static_cast<S*>(this));
	}

	void destroy() {
		if (!this->_is_destroyed) {
			this->_is_destroyed = true;
			this->_destroy();

			if (!this->_is_shared) {
				this->release();
			}
		}
	}

	template<typename F>
	void destroy(F&& func) {
		this->destroy_signal.connect(func);
		this->destroy();
	}

	node::signal<void()> destroy_signal;

protected:
#if 0
	virtual ~intrusive_ptr() {
		printf("%s::~intrusive_ptr\n", typeid(this).name());
	}
#else
	virtual ~intrusive_ptr() = default;
#endif

	void set_responsible_object(intrusive_ptr* parent) {
		assert(parent);

		std::swap(this->_responsible_object, parent);

		parent->retain();

		if (parent) {
			return parent->release();
		}
	}

	void retain() {
		if (this->_use_count >= std::numeric_limits<decltype(this->_use_count)>::max()) {
			assert(false);
			abort();
		}

		++this->_use_count;
	}

	void release() {
		assert(this->_use_count > 0);

		if (--this->_use_count == 0) {
			if (!this->_is_destroyed) {
				this->_is_destroyed = true;
				this->_destroy();

				if (this->_use_count > 0) {
					// sth. async happened in _destroy() --> wait until release() is called again
					return;
				}
			}

			if (this->_responsible_object) {
				this->_responsible_object->release();
			} else {
				delete this;
			}
		}
	}

	virtual void _destroy() {
		this->destroy_signal.emit();
	}
};


template<typename T>
class intrusive_ptr_wrapper : public T {
public:
	typedef T element_type;

	template<typename... Args>
	intrusive_ptr_wrapper(intrusive_ptr* o, Args&&... args) : element_type(std::forward<Args>(args)...) {
		this->set_responsible_object(o);
	}
};


template<typename T>
class shared_ptr {
	template<typename S, typename... Args>
	friend shared_ptr<S> make_shared(Args&&... args);

public:
	typedef T element_type;

private:
	element_type* _ptr;

public:
	constexpr shared_ptr() : _ptr(nullptr) {}
	constexpr shared_ptr(std::nullptr_t) : _ptr(nullptr) {}


	shared_ptr(const shared_ptr<element_type>& other) : _ptr(other._ptr) {
		if (this->_ptr) {
			this->_ptr->retain();
		}
	}

	shared_ptr(shared_ptr<element_type>&& other) {
		this->_ptr = other._ptr;
		other._ptr = nullptr;
	}

	shared_ptr<element_type>& operator=(const shared_ptr<element_type>& other) {
		this->_ptr = other._ptr;

		if (this->_ptr) {
			this->_ptr->retain();
		}

		return *this;
	}

	shared_ptr<element_type>& operator=(shared_ptr<element_type>&& other) {
		this->_ptr = other._ptr;
		other._ptr = nullptr;
		return *this;
	}

	shared_ptr<element_type>& operator=(std::nullptr_t) {
		this->reset();
		return *this;
	}

	~shared_ptr() {
		if (this->_ptr) {
			this->_ptr->release();
		}
	}

	typename std::add_lvalue_reference<element_type>::type operator*() const noexcept {
		return *this->_ptr;
	}

	element_type* operator->() const noexcept {
		return this->_ptr;
	}

	element_type* get() const noexcept {
		return this->_ptr;
	}

	explicit operator bool() const noexcept {
		return this->_ptr != nullptr;
	}

	decltype(_ptr->use_count()) use_count() const {
		return this->_ptr ? this->_ptr->use_count() : 0;
	}

	void reset() {
		if (this->_ptr) {
			this->_ptr->release();
		}

		this->_ptr = nullptr;
	}

private:
	explicit shared_ptr(element_type* o) : _ptr(o) {
		this->_ptr->_is_shared = true;
		// there is no need for a retain() here since it's guaranteed that o will have a use_count of exactly 1
	}
};




template<typename S, typename... Args>
shared_ptr<S> make_shared(Args&&... args) {
	S* o = new S(std::forward<Args>(args)...);
	return shared_ptr<S>(o);
}

} // namespace node

#endif // nodecc_object_h
