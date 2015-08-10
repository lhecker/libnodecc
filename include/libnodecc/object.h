#ifndef nodecc_object_h
#define nodecc_object_h

#include <cassert>
#include <type_traits>

#include "callback.h"
#include "signal.h"


namespace node {

/*
 * Reference counting base class for libnodecc.
 *
 * Due to the asynchronous programming model reference counting is needed
 * to comfortably ensure that an "object" will only get deleted if
 * it's currently not running an asynchronous callback etc.
 *
 * @attention Subclasses SHOULD make their destructor protected to enforce the usage of destroy() etc.
 */
class object {
	template<typename T>
	friend class shared_object;

private:
	object*      _responsible_object;
	unsigned int _use_count    = 1;
	bool         _is_destroyed = false;

public:
	struct Destructor {
		void operator()(object* o) {
			o->destroy();
		}
	};


	explicit object() : _responsible_object(nullptr) {}

	explicit object(object* other) : _responsible_object(other) {
		this->_responsible_object->retain();
	}

	object(object&&) = delete;

	decltype(_use_count) use_count() const {
		return this->_use_count;
	}

	void destroy() {
		this->_is_destroyed = true;
		this->_destroy();
		this->release();
	}

	template<typename F>
	void destroy(F&& func) {
		this->destroy_signal.connect(func);
		this->destroy();
	}

	node::signal<void()> destroy_signal;

protected:
	//virtual ~object() = default;
	virtual ~object() {
		printf("%s", typeid(this).name());
	}

	void set_responsible_object(object* parent) {
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

		this->_use_count++;
	}

	void release() {
		assert(this->_use_count > 0);

		this->_use_count--;

		if (this->_use_count == 0) {
			if (this->_responsible_object) {
				return this->_responsible_object->release();
			}

			if (!this->_is_destroyed) {
				this->_is_destroyed = true;
				this->_destroy();

				if (this->_use_count > 0) {
					// sth. async happened in _destroy() --> wait until release() is called again
					return;
				}
			}

			delete this;
		}
	}

	virtual void _destroy() {
		this->destroy_signal.emit();
	}
};


template<typename T>
class object_wrapper : public T {
public:
	typedef T element_type;

	template<typename... Args>
	object_wrapper(object* o, Args&&... args) : element_type(std::forward<Args>(args)...) {
		this->set_responsible_object(o);
	}
};


template<typename T>
class shared_object {
	template<typename S, typename... Args>
	friend shared_object<S> make_shared(Args&&... args);

public:
	typedef T element_type;


	explicit shared_object(element_type* o) : _ptr(o) {
		// there is no need for a retain() here since it's guaranteed that o will have a use_count of exactly 1
	}

	shared_object(const shared_object<element_type>& other) : _ptr(other._ptr) {
		this->_ptr->retain();
	}

	shared_object(shared_object<element_type>&& other) {
		this->_ptr = other._ptr;
		other._ptr = nullptr;
	}

	shared_object<element_type>& operator=(const shared_object<element_type>& other) {
		this->_ptr = other._ptr;
		this->_ptr->retain();
	}

	shared_object<element_type>& operator=(shared_object<element_type>&& other) {
		this->_ptr = other._ptr;
		other._ptr = nullptr;
	}

	~shared_object() {
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
		return this->_ptr;
	}

	decltype(object::_use_count) use_count() const {
		return this->_ptr ? this->_ptr->_use_count : 0;
	}

	void reset() {
		if (this->_ptr) {
			this->_ptr->release();
		}

		this->_ptr = nullptr;
	}

private:
	element_type* _ptr;
};




template<typename S, typename... Args>
shared_object<S> make_shared(Args&&... args) {
	S* o = new S(std::forward<Args>(args)...);
	return shared_object<S>(o);
}

} // namespace node

#endif // nodecc_object_h
