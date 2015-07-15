#ifndef nodecc_event_h
#define nodecc_event_h

#include <functional>
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

	template<class V>
	constexpr T value_or(V&& v) const & {
		return *this ? **this : std::forward<V>(v);
	}

	template<class V>
	constexpr T&& value_or(V&& v) const && {
		return *this ? **this : std::forward<V>(v);
	}
};


template<class T>
class event;

template<class R, class... Args>
class event<R(Args...)> {
public:
	typedef std::function<R(Args...)> function_type;


	constexpr event() {}
	~event() {}

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

private:
	function_type _f;
};

} // namespace node

#endif // nodecc_event_h
