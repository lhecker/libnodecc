#ifndef nodecc_event_h
#define nodecc_event_h

#include <functional>
#include <stdexcept>
#include <utility>

#include "buffer.h"


namespace node {

template<typename T>
struct event_optional : std::pair<T, bool> {
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

/*
 * Bugs in VS2015:
 * Using
 *   template<typename R, typename... Args>
 *   class event<R(Args...)> {}
 * does not compile.
 * Even without using template specialization on the class,
 * subsequent usage of "Args..." as parameter pack won't work either.
 */
template<class T>
class event;

template<class R, class... Args>
class event<R(Args...)> {
public:
	typedef std::function<R(Args...)> type;
	typedef event_optional<node::buffer> result_type;


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

	result_type emit(Args... args) {
		if (this->empty()) {
			return result_type();
		} else {
			this->_f(std::forward<Args>(args)...);
			return result_type();
		}
	}

	void swap(event<R(Args...)>& other) noexcept {
		std::swap(this->_f, other._f);
	}

private:
	type _f;
};

} // namespace node

#endif // nodecc_event_h
