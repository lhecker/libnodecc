#ifndef nodecc_signal_h
#define nodecc_signal_h

#include <functional>
#include <mutex>
#include <type_traits>


namespace node {

struct signal_tag {};


namespace detail {

template<typename... Args>
struct connection_base {
	connection_base() = default;
	connection_base(connection_base&&) = delete;

	virtual ~connection_base() = default;

	virtual bool emit(Args&&... args) = 0;

	connection_base* next = nullptr;
};

template<typename... Args>
struct tagged_connection_base : connection_base<Args...> {
	constexpr tagged_connection_base(const signal_tag& tag) : tag(tag) {}

	const signal_tag& tag;
};

template<typename F, typename... Args>
struct basic_connection : connection_base<Args...> {
	basic_connection(F&& func) : func(std::forward<F>(func)) {}

	bool emit(Args&&... args) override {
		this->func(std::forward<Args>(args)...);
		return false;
	}

	F func;
};

template<typename F, typename... Args>
struct tagged_connection : tagged_connection_base<Args...> {
	tagged_connection(const signal_tag& tag, F&& func) : tagged_connection_base<Args...>(tag), func(std::forward<F>(func)) {}

	bool emit(Args&&... args) override {
		this->func(std::forward<Args>(args)...);
		return false;
	}

	F func;
};

template<typename S, typename F, typename... Args>
struct tracked_connection : connection_base<Args...> {
	tracked_connection(S&& tracked_object, F&& func) : tracked_object(std::forward<S>(tracked_object)), func(std::forward<F>(func)) {}

	bool emit(Args&&... args) override {
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

}


template<typename T>
class signal;

template<typename R, typename... Args>
class signal<R(Args...)> {
	// TODO: should sth. like .emit_and_collect() be implemented?
	static_assert(std::is_void<R>::value, "node::signal requires the return value to be void");

public:
	typedef detail::connection_base<Args...> connection_type;

	constexpr signal() : _head(nullptr), _tail(nullptr) {}

	signal(const signal&) = delete;
	signal& operator=(const signal&) = delete;

	~signal() {
		this->clear();
	}


	constexpr operator bool() const noexcept {
		return this->_head != nullptr;
	}

	constexpr bool empty() const noexcept {
		return this->_head == nullptr;
	}

	template<typename F>
	void* connect(F func) {
		auto entry = new detail::basic_connection<F, Args...>(std::forward<F>(func));
		this->_append(entry);
		return entry;
	}

	template<typename F>
	void* tagged_connect(const signal_tag& tag, F func) {
		auto entry = new detail::tagged_connection<F, Args...>(tag, std::forward<F>(func));
		this->_append(entry);
		return entry;
	}

	template<typename S, typename F>
	void* tracked_connect(S tracked_object, F func) {
		auto entry = new detail::tracked_connection<S, F, Args...>(std::forward<S>(tracked_object), std::forward<F>(func));
		this->_append(entry);
		return entry;
	}

	bool emit(Args... args) {
		connection_type* prev = nullptr;
		connection_type* ptr = this->_head;

		while (ptr) {
			connection_type* next = ptr->next;
			const bool remove = ptr->emit(std::forward<Args>(args)...);

			if (remove) {
				this->_remove(prev, ptr);
			} else {
				prev = ptr;
			}

			ptr = next;
		}

		return prev != nullptr;
	}

	bool emit_and_clear(Args... args) {
		connection_type* ptr = this->_head;
		bool ret = ptr != nullptr;

		// see clear() as to why we need to set it to null
		this->_head = nullptr;
		this->_tail = nullptr;

		while (ptr) {
			connection_type* tmp = ptr;
			ptr = ptr->next;

			tmp->emit(std::forward<Args>(args)...);

			delete tmp;
		}

		return ret;
	}

	void disconnect(const void* iter) {
		if (iter) {
			auto citer = static_cast<const connection_type*>(iter);

			connection_type* prev = nullptr;
			connection_type* ptr = this->_head;

			while (ptr) {
				if (citer == ptr) {
					this->_remove(prev, ptr);
				}

				prev = ptr;
				ptr = ptr->next;
			}
		}
	}

	void disconnect(const signal_tag& tag) {
		connection_type* prev = nullptr;
		connection_type* ptr = this->_head;

		while (ptr) {
			const auto* cptr = dynamic_cast<detail::tagged_connection_base<Args...>*>(ptr);

			if (cptr && cptr->tag == tag) {
				this->_remove(prev, ptr);
			}

			prev = ptr;
			ptr = ptr->next;
		}
	}

	void clear() {
		const connection_type* ptr = this->_head;

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
			const connection_type* tmp = ptr;
			ptr = ptr->next;
			delete tmp;
		}
	}

	void swap(signal<R(Args...)>& other) noexcept {
		std::swap(this->_head, other._head);
		std::swap(this->_tail, other._tail);
	}

protected:
	void _append(connection_type* ptr) {
		if (this->_tail) {
			this->_tail->next = ptr;
		} else {
			this->_head = ptr;
		}

		this->_tail = ptr;
	}

	void _remove(connection_type* prev, connection_type* ptr) {
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

	connection_type* _head;
	connection_type* _tail;
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

	void remove(typename signal<R(Args...)>::connection_type* iter) {
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
