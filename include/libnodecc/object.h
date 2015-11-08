#ifndef nodecc_object_h
#define nodecc_object_h

#include <cassert>

#include "events.h"


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
class object : public events::emitter {
private:
	template<typename T>
	friend class node::shared_ptr;

public:
	static const node::events::symbol<void()> destroy_event;

	explicit object() : _responsible_object(nullptr), _use_count(1), _is_destroyed(0) {}

	object(object&&) = delete;

	size_t use_count() const noexcept {
		return this->_use_count;
	}

	template<typename S>
	node::shared_ptr<S> shared_from_this() {
		return node::shared_ptr<S>(static_cast<S*>(this));
	}

	void destroy() {
		// prevent double destroy()
		if (!this->_is_destroyed) {
			this->_is_destroyed = 1;
			this->_destroy();

			/*
			 * We don't need to release() if we don't have a responsible_object and
			 * thus are shared, since the shared_ptr will call release for us already.
			 */
			if (this->_responsible_object) {
				this->release();
			}
		}
	}

protected:
	virtual ~object() = default;

	void set_responsible_object(object* parent) {
		std::swap(this->_responsible_object, parent);

		if (this->_responsible_object) {
			this->_responsible_object->retain();
		}

		if (parent) {
			parent->release();
		}
	}

	void retain() {
		++this->_use_count;
	}

	void release() {
		assert(this->_use_count > 0);

		if (--this->_use_count == 0) {
			if (!this->_is_destroyed) {
				this->_is_destroyed = 1;
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
		this->emit(destroy_event);
		this->removeAllListeners();
	}

private:
	object* _responsible_object;
	size_t  _use_count    : 31;
	size_t  _is_destroyed : 1;
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

	size_t use_count() const {
		return this->_ptr ? this->_ptr->use_count() : 0;
	}

	void reset() {
		if (this->_ptr) {
			this->_ptr->release();
		}

		this->_ptr = nullptr;
	}

private:
	constexpr shared_ptr(element_type* o) : _ptr(o) {
		// there is no need for a retain() here since it's guaranteed that o will have a use_count of exactly 1
	}
};




template<typename S, typename... Args>
shared_ptr<S> make_shared(Args&&... args) noexcept(false) {
	S* o = new S(std::forward<Args>(args)...);
	return shared_ptr<S>(o);
}

} // namespace node

#endif // nodecc_object_h
