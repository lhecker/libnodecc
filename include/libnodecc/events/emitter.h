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
	struct iterator {
		constexpr iterator(event_handler_base* ptr) : _curr(ptr), _prev(nullptr) {}
		constexpr iterator(event_handler_base* curr, event_handler_base* prev) : _curr(curr), _prev(prev) {}

		event_handler_base& operator*() const noexcept {
			return *this->_curr;
		}

		event_handler_base* operator->() const noexcept {
			return this->_curr;
		}

		event_handler_root::iterator& operator++() noexcept {
			this->_prev = this->_curr;
			this->_curr = this->_curr->next;
			return *this;
		}

		event_handler_root::iterator operator++(int) noexcept {
			iterator it(this->_curr->next, this->_curr);
			++(*this);
			return it;
		}

		friend bool operator==(const iterator& lhs, const iterator& rhs) noexcept {
			return lhs._curr == rhs._curr;
		}

		friend bool operator!=(const iterator& lhs, const iterator& rhs) noexcept {
			return lhs._curr != rhs._curr;
		}

		event_handler_base* _curr;
		event_handler_base* _prev;
	};

	constexpr event_handler_root() : head(nullptr), tail(nullptr) {}

	event_handler_root(const event_handler_root&) = delete;
	event_handler_root& operator=(const event_handler_root&) = delete;

	~event_handler_root();

	void push_back(event_handler_base* ptr) noexcept;
	void erase(iterator it) noexcept;

	iterator begin() const noexcept {
		return iterator(this->head);
	}

	iterator end() const noexcept {
		return iterator(nullptr);
	}

	event_handler_base* head;
	event_handler_base* tail;
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
		auto& root = this->_events[(void*)&type];
		auto ptr = new detail::basic_event_handler<F, T>(std::forward<F>(func));
		root.push_back(ptr);
		return ptr;
	}

	template<typename T, typename... Args>
	void emit(const events::type<T>& type, Args&& ...args) {
		const auto& it = this->_events.find((void*)&type);

		if (it != this->_events.cend()) {
			const auto rend = it->second.end();
			auto rit = it->second.begin();

			this->_locked_type = (void*)&type;

			while (rit != rend) {
				const bool remove = static_cast<detail::event_handler<T>&>(*rit).emit(std::forward<Args>(args)...);

				/*
				 * If someone tries to call off()/removeAllListeners(...) inside the callback,
				 * we need to ensure that the root is deleted *after* the callback ended.
				 * The code below fulfills this requirement in combination with the 3 remove methods.
				 */
				if (this->_locked_type != (void*)&type) {
					if (this->_locked_type == kEraseAll) {
						this->_events.clear();
					} else {
						this->_events.erase(it);
					}

					return;
				}

				if (remove) {
					it->second.erase(rit++);
				} else {
					++rit;
				}
			}

			this->_locked_type = nullptr;

			if (it->second.head == nullptr) {
				this->_events.erase(it);
			}
		}
	}

	void off(const events::detail::base_type& type, void* iter);
	void removeAllListeners(const events::detail::base_type& type);
	void removeAllListeners();

private:
	static const void* kEraseAll;

	std::map<void*, detail::event_handler_root> _events;
	void* _locked_type;
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
