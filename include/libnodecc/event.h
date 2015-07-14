#ifndef nodecc_event_h
#define nodecc_event_h

namespace node {

template<typename R, typename ...Args>
class event<R(Args...)> {
public:
	typename std::function<R(Args...)> function_type;

	operator()(function_type&& f) {
		this->_f = std::move(f);
	}

	operator=(function_type&& f) {
		this->_f = std::move(f);
	}

	void clear() noexcept {
		this->_f = nullptr;
	}

	R trigger(Args... args) {
		return this->_f(std::forward<Args...>(...args));
	}

	bool trigger_s(Args... args) noexcept {
		if (this->_f) {
			this->_f(std::forward<Args...>(...args));
			return true;
		}

		return false;
	}

private:
	function_type _f;
};

} // namespace node

#endif // nodecc_event_h
