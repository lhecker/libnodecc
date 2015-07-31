#ifndef nodecc_event_h
#define nodecc_event_h

#include <functional>
#include <mutex>
#include <stdexcept>
#include <utility>


namespace node {

template<typename T>
struct event_optional : std::pair<T, bool> {
	using std::pair<T, bool>::pair;

	constexpr operator bool() const noexcept {
		return this->second;
	}

	constexpr T* operator->() const {
		return &this->first;
	}

	constexpr T& operator*() const & {
		return this->first;
	}

	T&& operator*() && {
		return this->first;
	}

	constexpr const T& value() const & {
		return this->first;
	}

	T&& value() && {
		return this->first;
	}

	template<typename V>
	constexpr T value_or(V&& v) const & {
		return *this ? **this : std::forward<V>(v);
	}

	template<typename V>
	constexpr T&& value_or(V&& v) const && {
		return *this ? **this : std::forward<V>(v);
	}
};


template<typename T>
class event;

template<typename R, typename... Args>
class event<R(Args...)> {
public:
	typedef std::function<R(Args...)> function_type;


	constexpr event() {}

	event(const event&) = delete;
	event& operator=(const event&) = delete;


	template<typename F>
	void operator()(F&& f) {
		this->_f = std::forward<F>(f);
	}

	constexpr operator bool() const noexcept {
		return static_cast<bool>(this->_f);
	}

	constexpr bool empty() const noexcept {
		return !this->_f;
	}

	void clear() noexcept {
		this->_f = nullptr;
	}

	void swap(event<R(Args...)>& other) noexcept {
		std::swap(this->_f, other._f);
	}

	template<typename S = R, typename = typename std::enable_if<std::is_void<S>::value>::type>
	bool emit(Args... args) {
		if (this->empty()) {
			return false;
		} else {
			this->_f(std::forward<Args>(args)...);
			return true;
		}
	}

	template<typename S = R, typename = typename std::enable_if<!std::is_void<S>::value>::type>
	event_optional<S> emit(Args... args) {
		if (this->empty()) {
			return event_optional<S>();
		} else {
			return event_optional<S>(this->_f(std::forward<Args>(args)...), true);
		}
	}

protected:
	function_type _f;
};


namespace detail {

template<typename M, typename T>
class locking_event;

template<typename M, typename R, typename... Args>
class locking_event<M, R(Args...)> : public event<R(Args...)> {
public:
	template<typename F>
	void operator()(F&& f) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		event<R(Args...)>::operator()(std::forward<F>(f));
	}

	operator bool() const noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return event<R(Args...)>::operator bool();
	}

	bool empty() const noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return event<R(Args...)>::empty();
	}

	void clear() noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		event<R(Args...)>::clear();
	}

	template<typename S = R, typename = typename std::enable_if<std::is_void<S>::value>::type>
	bool emit(Args... args) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return event<R(Args...)>::emit(std::forward<Args>(args)...);
	}

	template<typename S = R, typename = typename std::enable_if<!std::is_void<S>::value>::type>
	event_optional<S> emit(Args... args) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return event<R(Args...)>::emit(std::forward<Args>(args)...);
	}

protected:
	locking_event(M m) : _m(m) {}

	M _m;
};

} // namespace detail


template<typename T>
class threadsafe_event;

template<typename R, typename... Args>
class threadsafe_event<R(Args...)> : public detail::locking_event<std::mutex, R(Args...)> {
};


template<typename M, typename T>
class locking_event;

template<typename M, typename R, typename... Args>
class locking_event<M, R(Args...)> : public detail::locking_event<M&, R(Args...)> {
public:
	locking_event(M& m) : detail::locking_event<M&, R(Args...)>(m) {}
};

} // namespace node

#endif // nodecc_event_h
