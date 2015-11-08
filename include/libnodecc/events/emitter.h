#ifndef nodecc_events_emitter_h
#define nodecc_events_emitter_h

#include <map>
#include <type_traits>

#include "symbol.h"


namespace node {
namespace events {
namespace detail {

struct event_handler_base {
	static constexpr std::size_t delete_flag = size_t(1) << (std::numeric_limits<std::size_t>::digits - 1);

	constexpr event_handler_base() : next(nullptr), ref_count(0) {}

	event_handler_base(const event_handler_base&) = delete;
	event_handler_base& operator=(const event_handler_base&) = delete;

	virtual ~event_handler_base() = default;

	event_handler_base* next;
	std::size_t ref_count;
};

template<typename T>
struct event_handler;

template<typename R, typename... Args>
struct event_handler<R(Args...)> : event_handler_base {
	virtual void emit(Args ...args) = 0;
};

template<typename F, typename T>
struct basic_event_handler;

template<typename F, typename R, typename... Args>
struct basic_event_handler<F, R(Args...)> : event_handler<R(Args...)> {
	basic_event_handler(F&& func) : event_handler<R(Args...)>(), func(std::forward<F>(func)) {}

	void emit(Args ...args) override {
		this->func(std::forward<Args>(args)...);
	}

	F func;
};

class event_handler_root {
public:
	struct iterator : std::iterator<std::forward_iterator_tag, event_handler_base> {
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
 * - This class (as emitter<void(size_t)>, using -O3 and only a single symbol entry)
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
	void* on(const events::symbol<T>& symbol, F&& func) {
		auto& root = this->_events[(void*)&symbol];
		auto ptr = new detail::basic_event_handler<F, T>(std::forward<F>(func));
		root.push_back(ptr);
		return ptr;
	}

	template<typename T, typename... Args>
	void emit(const events::symbol<T>& symbol, Args&& ...args) {
		const auto& it = this->_events.find((void*)&symbol);

		if (it != this->_events.cend()) {
			const auto rend = it->second.end();
			auto rit = it->second.begin();

			while (rit != rend) {
				detail::event_handler<T>& handler = static_cast<detail::event_handler<T>&>(*rit);

				// Guard the handler
				++handler.ref_count;

				// Call the callback! Remember: The callback might call off() or removeAllListeners() recursively!
				handler.emit(std::forward<Args>(args)...);

				// Advance the iterator before we possibly delete it below
				++rit;

				// Delete the handler if it has been marked and we're the last emit() in the callstack to use it
				if (--handler.ref_count == detail::event_handler_base::delete_flag) {
					delete &handler;
				}
			}

			/*
			 * If it->second.head is nullptr we should actually delete the root,
			 * but we can't do that since the root might already have been deleted
			 * using removeAllListeners() by one of the callbacks of the handlers above.
			 */
		}
	}

	bool has_listener(const events::detail::base_symbol& symbol) const;
	std::size_t listener_count(const events::detail::base_symbol& symbol) const;

	void off(const events::detail::base_symbol& symbol, void* iter);
	void removeAllListeners(const events::detail::base_symbol& symbol);
	void removeAllListeners();

private:
	static const void* kEraseAll;

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
