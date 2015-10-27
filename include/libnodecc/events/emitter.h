#ifndef nodecc_events_emitter_h
#define nodecc_events_emitter_h

#include <map>
#include <type_traits>

#include "type.h"


namespace node {
namespace events {
namespace detail {

struct event_handler_base {
	constexpr event_handler_base() : next(nullptr) {}

	event_handler_base(const event_handler_base&) = delete;
	event_handler_base& operator=(const event_handler_base&) = delete;

	virtual ~event_handler_base() = default;

	event_handler_base* next;
};

template<typename T>
struct event_handler;

template<typename R, typename... Args>
struct event_handler<R(Args...)> : event_handler_base {
	virtual bool emit(Args ...args) = 0;
};

template<typename F, typename T>
struct basic_event_handler;

template<typename F, typename R, typename... Args>
struct basic_event_handler<F, R(Args...)> : event_handler<R(Args...)> {
	basic_event_handler(F&& func) : event_handler<R(Args...)>(), func(std::forward<F>(func)) {}

	bool emit(Args ...args) override {
		this->func(std::forward<Args>(args)...);
		return false;
	}

	F func;
};

class event_handler_root {
public:
	constexpr event_handler_root() : _head(nullptr), _tail(nullptr) {}

	event_handler_root(const event_handler_root&) = delete;
	event_handler_root& operator=(const event_handler_root&) = delete;

	~event_handler_root() {
		auto ptr = this->_head;

		// set head to nullptr early to prevent recursive calls to clear()
		this->_head = nullptr;
		this->_tail = nullptr;

		while (ptr) {
			const auto tmp = ptr;
			ptr = ptr->next;
			delete tmp;
		}
	}

	template<typename T, typename F>
	void* on(F&& func) {
		auto ptr = new basic_event_handler<F, T>(std::forward<F>(func));
		this->_append(ptr);
		return ptr;
	}

	bool off(void* iter) {
		if (iter) {
			event_handler_base* prev = nullptr;
			event_handler_base* ptr = this->_head;

			while (ptr) {
				if (ptr == iter) {
					return this->_remove(prev, ptr);
				}

				prev = ptr;
				ptr = ptr->next;
			}
		}

		return false;
	}

	template<typename T, typename... Args>
	bool emit(Args&& ...args) {
		typedef event_handler<T> event_handler_type;

		event_handler_type* prev = nullptr;
		event_handler_type* ptr = static_cast<event_handler_type*>(this->_head);

		while (ptr) {
			event_handler_type* next = static_cast<event_handler_type*>(ptr->next);
			const bool remove = ptr->emit(std::forward<Args>(args)...);

			if (remove) {
				this->_remove(prev, ptr);
			} else {
				prev = ptr;
			}

			ptr = next;
		}

		return this->_head == nullptr;
	}

private:
	void _append(event_handler_base* ptr) {
		if (this->_tail) {
			this->_tail->next = ptr;
		} else {
			this->_head = ptr;
		}

		this->_tail = ptr;
	}

	bool _remove(event_handler_base* prev, event_handler_base* ptr) {
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

		return this->_head == nullptr;
	}

	event_handler_base* _head;
	event_handler_base* _tail;
};

} // namespace detail


/*
 * - This class (as emitter<void(size_t)>, using -O3 and only a single type entry)
 *   manages to invoke up to 250 million event handlers per second on a i7 3770
 *   and is thus only about 17% slower than a pure std::forward_list<std::function<void(int)>>.
 *   (The std::function list iteration was by the way completely inlined while the emitter was not.)
 * - If NODE_EVENTS_FAST_TRACK is used the performance becomes indistinguishable - the difference is below 5%.
 * - Furthermore std::map was choosen over std::unordered_map since it offers a better performance
 *   for maps with less then about 16 entries, which should be the general usecase.
 *
 * In my opinion this should be enough for almost all usecases.
 */
class emitter {
public:
	template<typename T, typename F>
	void* on(const events::type<T>& type, F&& func) {
		return this->_events[(void*)&type].on<T>(std::forward<F>(func));
	}

	template<typename T>
	void off(const events::type<T>& type, void* iter) {
		const auto& it = this->_events.find((void*)&type);

		if (it != this->_events.cend() && it->second.off(iter)) {
			this->_events.erase(it);
		}
	}

	template<typename T, typename... Args>
	void emit(const events::type<T>& type, Args&& ...args) {
		const auto& it = this->_events.find((void*)&type);

		if (it != this->_events.cend() && it->second.emit<T>(std::forward<Args>(args)...)) {
			this->_events.erase(it);
		}
	}

	template<typename T>
	void removeAllListeners(const events::type<T>& type) {
		this->_events.erase((void*)&type);
	}

	void removeAllListeners() {
		this->_events.clear();
	}

private:
	std::map<void*, detail::event_handler_root> _events;
};


#define NODE_EVENT_ENABLE_FAST_TRACK() \
	public:                            \
		using emitter::on;             \
		using emitter::off;            \
		using emitter::emit;           \

#define NODE_EVENTS_ADD_FAST_TRACK(_name_, ...)                                 \
	private:                                                                    \
		node::events::detail::event_handler_root _name_##_root;                 \
		                                                                        \
	public:                                                                     \
		struct _name_##t {};                                                    \
		static constexpr _name_##t _name_ = _name_##t();                        \
		                                                                        \
		template<typename F>                                                    \
		void* on(_name_##t, F&& func) {                                         \
			return this->_name_##_root.on<__VA_ARGS__>(std::forward<F>(func));  \
		}                                                                       \
		                                                                        \
		void off(_name_##t, void* iter) {                                       \
			this->_name_##_root.off(iter);                                      \
		}                                                                       \
		                                                                        \
		template<typename... Args>                                              \
		void emit(_name_##t, Args&& ...args) {                                  \
			this->_name_##_root.emit<__VA_ARGS__>(std::forward<Args>(args)...); \
		}                                                                       \

} // namespace events
} // namespace node

#endif // nodecc_events_emitter_h
