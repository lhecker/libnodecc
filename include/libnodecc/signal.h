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

	virtual bool operator()(Args... args) = 0;

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
	typedef detail::signal_element_base<Args...>* iterator;


	constexpr signal() {}

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
		struct signal_element : detail::signal_element_base<Args...> {
			signal_element(F&& func) : detail::signal_element_base<Args...>(), func(std::forward<F>(func)) {}

			bool operator()(Args... args) override {
				this->func(std::forward<Args>(args)...);
				return false;
			}

			F func;
		};

		this->_append(new signal_element(std::forward<F>(func)));
	}

	template<typename S, typename F>
	void tracked_connect(S&& tracked_object, F&& func) {
		struct signal_element : detail::signal_element_base<Args...> {
			signal_element(S&& tracked_object, F&& func) : detail::signal_element_base<Args...>(), tracked_object(std::forward<S>(tracked_object)), func(std::forward<F>(func)) {}

			bool operator()(Args... args) override {
				const auto locked_object = this->tracked_object.lock();

				if (locked_object) {
					func(std::forward<Args>(args)...);
					return false;
				} else {
					return true;
				}
			}

			std::weak_ptr<S> tracked_object;
			F func;
		};

		this->_append(new signal_element(std::forward<S>(tracked_object), std::forward<F>(func)));
	}

	bool emit(Args... args) {
		if (this->empty()) {
			return false;
		} else {
			iterator prev = nullptr;
			iterator ptr = this->_head;

			while (ptr) {
				const bool remove = ptr->operator()(std::forward<Args>(args)...);

				if (remove) {
					this->_remove(prev, ptr);
				}

				prev = ptr;
				ptr = ptr->next;
			}

			return true;
		}
	}

	void remove(iterator iter) {
		iterator prev = nullptr;
		iterator ptr = this->_head;

		while (ptr) {
			if (iter == ptr) {
				this->_remove(prev, ptr);
			}

			prev = ptr;
			ptr = ptr->next;
		}
	}

	void clear() {
		iterator ptr = this->_head;

		while (ptr) {
			const iterator tmp = ptr;
			ptr = ptr->next;
			delete tmp;
		}

		this->_head = nullptr;
		this->_tail = nullptr;
	}

	void swap(signal<R(Args...)>& other) noexcept {
		std::swap(this->_head, other._head);
		std::swap(this->_tail, other._tail);
	}

protected:
	void _append(iterator ptr) {
		if (this->_tail) {
			this->_tail->next = ptr;
		} else {
			this->_head = ptr;
		}

		this->_tail = ptr;
	}

	void _remove(iterator prev, iterator ptr) {
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

	iterator _head;
	iterator _tail;
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

	void remove(typename signal<R(Args...)>::iterator iter) {
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
