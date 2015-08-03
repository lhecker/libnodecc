#ifndef nodecc_callback_h
#define nodecc_callback_h

#include <functional>
#include <mutex>
#include <type_traits>


namespace node {

template<typename T>
struct callback_optional : std::pair<T, bool> {
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
class callback;

template<typename R, typename... Args>
class callback<R(Args...)> {
public:
	typedef std::function<R(Args...)> callback_type;


	constexpr callback() {}

	callback(const callback&) = delete;
	callback& operator=(const callback&) = delete;


	constexpr operator bool() const noexcept {
		return static_cast<bool>(this->_f);
	}

	constexpr bool empty() const noexcept {
		return !this->_f;
	}

	template<typename F>
	void connect(F&& func) {
		this->_f = std::forward<F>(func);
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
	callback_optional<S> emit(Args... args) {
		if (this->empty()) {
			return callback_optional<S>();
		} else {
			return callback_optional<S>(this->_f(std::forward<Args>(args)...), true);
		}
	}

	void clear() noexcept {
		this->_f = nullptr;
	}

	void swap(callback<R(Args...)>& other) noexcept {
		std::swap(this->_f, other._f);
	}

protected:
	callback_type _f;
};


namespace detail {

template<typename M, typename T>
class locking_callback;

template<typename M, typename R, typename... Args>
class locking_callback<M, R(Args...)> : public callback<R(Args...)> {
public:
	template<typename F>
	void connect(F&& func) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		callback<R(Args...)>::connect(std::forward<F>(func));
	}

	operator bool() const noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return callback<R(Args...)>::operator bool();
	}

	bool empty() const noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return callback<R(Args...)>::empty();
	}

	template<typename S = R, typename = typename std::enable_if<std::is_void<S>::value>::type>
	bool emit(Args... args) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return callback<R(Args...)>::emit(std::forward<Args>(args)...);
	}

	template<typename S = R, typename = typename std::enable_if<!std::is_void<S>::value>::type>
	callback_optional<S> emit(Args... args) {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		return callback<R(Args...)>::emit(std::forward<Args>(args)...);
	}

	void clear() noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock(this->_m);
		callback<R(Args...)>::clear();
	}

	void swap(callback<R(Args...)>& other) noexcept {
		std::lock_guard<typename std::remove_reference<M>::type> lock1(this->_m);
		std::lock_guard<typename std::remove_reference<M>::type> lock2(other._m);
		callback<R(Args...)>::swap(other);
	}

protected:
	locking_callback(M m) : _m(m) {}

	M _m;
};

} // namespace detail


template<typename T>
class threadsafe_callback;

template<typename R, typename... Args>
class threadsafe_callback<R(Args...)> : public detail::locking_callback<std::mutex, R(Args...)> {
};


template<typename M, typename T>
class locking_callback;

template<typename M, typename R, typename... Args>
class locking_callback<M, R(Args...)> : public detail::locking_callback<M&, R(Args...)> {
public:
	locking_callback(M& m) : detail::locking_callback<M&, R(Args...)>(m) {}
};

} // namespace node

#endif // nodecc_callback_h
