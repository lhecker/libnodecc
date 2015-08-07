#ifndef nodecc_signal_h
#define nodecc_signal_h

#include <functional>
#include <mutex>
#include <type_traits>


namespace node {

namespace detail {

template<typename... Args>
struct signal_element_base {
	constexpr signal_element_base() : next(nullptr) {}
	virtual ~signal_element_base() {};

	virtual bool emit(Args... args) = 0;

	signal_element_base* next;
};

}


template<typename T>
class signal;

template<typename R, typename... Args>
class signal<R(Args...)> {
	// TODO: should sth. like .emit_and_collect() be implemented?
	static_assert(std::is_void<R>::value, "node::signal requires the return value to be void");

public:
	typedef detail::signal_element_base<Args...> list_type;


	constexpr signal() : _head(nullptr), _tail(nullptr) {}

	signal(const signal&) = delete;
	signal& operator=(const signal&) = delete;

	~signal() {
		this->clear();
	}


	constexpr operator bool() const noexcept {
		return this->_head;
	}

	constexpr bool empty() const noexcept {
		return !this->_head;
	}

	template<typename F>
	void connect(F&& func) {
		struct signal_element : list_type {
			signal_element(F&& func) : list_type(), func(std::forward<F>(func)) {}

			bool emit(Args... args) override {
				this->func(std::forward<Args>(args)...);
				return false;
			}

			F func;
		};

		this->_append(new signal_element(std::forward<F>(func)));
	}

	template<typename S, typename F>
	void tracked_connect(S&& tracked_object, F&& func) {
		struct signal_element : list_type {
			signal_element(S&& tracked_object, F&& func) : list_type(), tracked_object(std::forward<S>(tracked_object)), func(std::forward<F>(func)) {}

			bool emit(Args... args) override {
				// lock() to ensure that the object is not deleted during the func() call
				const auto locked_object = this->tracked_object.lock();

				if (locked_object) {
					func(std::forward<Args>(args)...);
					return false;
				} else {
					return true;
				}
			}

			std::weak_ptr<typename std::remove_reference<S>::type::element_type> tracked_object;
			F func;
		};

		this->_append(new signal_element(std::forward<S>(tracked_object), std::forward<F>(func)));
	}

	bool emit(Args... args) {
		list_type* prev = nullptr;
		list_type* ptr = this->_head;

		while (ptr) {
			list_type* next = ptr->next;
			const bool remove = ptr->emit(std::forward<Args>(args)...);

			if (remove) {
				this->_remove(prev, ptr);
			}

			prev = ptr;
			ptr = next;
		}

		return prev != nullptr;
	}

	bool emit_and_clear(Args... args) {
		list_type* ptr = this->_head;
		bool ret = ptr != nullptr;

		// see clear() as to why we need to set it to null
		this->_head = nullptr;
		this->_tail = nullptr;

		while (ptr) {
			list_type* tmp = ptr;
			ptr = ptr->next;

			tmp->emit(std::forward<Args>(args)...);

			delete tmp;
		}

		return ret;
	}

	void remove(list_type* iter) {
		list_type* prev = nullptr;
		list_type* ptr = this->_head;

		while (ptr) {
			if (iter == ptr) {
				this->_remove(prev, ptr);
			}

			prev = ptr;
			ptr = ptr->next;
		}
	}

	void clear() {
		const list_type* ptr = this->_head;

		/*
		 * If this signal is wrapped in a class managed by a shared_ptr,
		 * which is only held by a callable object held by this signal,
		 * calling clear() will lead to this destructor getting called,
		 * which in turn calls clear() again.
		 * ---> We need to make sure that clear() is doing nothing there.
		 */
		this->_head = nullptr;
		this->_tail = nullptr;

		while (ptr) {
			const list_type* tmp = ptr;
			ptr = ptr->next;
			delete tmp;
		}
	}

	void swap(signal<R(Args...)>& other) noexcept {
		std::swap(this->_head, other._head);
		std::swap(this->_tail, other._tail);
	}

protected:
	void _append(list_type* ptr) {
		if (this->_tail) {
			this->_tail->next = ptr;
		} else {
			this->_head = ptr;
		}

		this->_tail = ptr;
	}

	bool _emit(bool clear, list_type* ptr, Args... args) {
		list_type* prev = nullptr;

		while (ptr) {
			list_type* next = ptr->next;
			const bool remove = ptr->emit(std::forward<Args>(args)...);

			if (clear) {
				delete ptr;
			} else if (remove) {
				this->_remove(prev, ptr);
			}

			prev = ptr;
			ptr = next;
		}

		return prev != nullptr;
	}

	void _remove(list_type* prev, list_type* ptr) {
		const auto next = ptr->next;

		if (prev) {
			prev->next = next;

			if (!next) {
				this->_tail = prev;
			}
		} else {
			// prev is null --> ptr is _head
			this->_head = next;

			if (!next) {
				this->_tail = nullptr;
			}
		}

		delete ptr;
	}

	list_type* _head;
	list_type* _tail;
};


namespace detail {

template<typename M, typename T>
class locking_signal;

template<typename M, typename R, typename... Args>
class locking_signal<M, R(Args...)> : public signal<R(Args...)> {
public:
	operator bool() const noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::operator bool();
	}

	bool empty() const noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::empty();
	}

	template<typename F>
	void connect(F&& func) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::connect(std::forward<F>(func));
	}

	template<typename S, typename F>
	void tracked_connect(S&& tracked_object, F&& func) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::tracked_connect(std::forward<S>(tracked_object), std::forward<F>(func));
	}

	bool emit(Args... args) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::emit(std::forward<Args>(args)...);
	}

	bool emit_and_clear(Args... args) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::emit(std::forward<Args>(args)...);
	}

	void remove(typename signal<R(Args...)>::list_type* iter) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::remove(iter);
	}

	void clear() {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		signal<R(Args...)>::clear();
	}

	void swap(signal<R(Args...)>& other) noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock1(this->_m);
		std::lock_guard<typename std::remove_reference<M>::type> lock2(other._m);
		signal<R(Args...)>::swap(other);
	}


protected:
	locking_signal(M m) : _m(m) {}

	M _m;
};

} // namespace detail


template<typename T>
class threadsafe_signal;

template<typename R, typename... Args>
class threadsafe_signal<R(Args...)> : public detail::locking_signal<std::mutex, R(Args...)> {
};


template<typename M, typename T>
class locking_signal;

template<typename M, typename R, typename... Args>
class locking_signal<M, R(Args...)> : public detail::locking_signal<M&, R(Args...)> {
public:
	locking_signal(M& m) : detail::locking_signal<M&, R(Args...)>(m) {}
};

} // namespace node

#endif // nodecc_signal_h
