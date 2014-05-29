#ifndef nodecc_events_event_emitter_h
#define nodecc_events_event_emitter_h

#include <functional>
#include <list>
#include <unordered_map>

namespace events {

/*
 * A common event emitter.
 *
 * TODO: Strings will be hashed and registered with their hash value.
 *       This will lead to collisions.
 *
 * Use it carefully since it casts between std::function objects freely
 * and therefore circumvents type safety of the functional template.
 *
 * Usage Example:
 *   std::function<void(const std::string &)> cb = [](const std::string &str) {};
 *   event_emitter.on(12345, cb); // you can use a prehashed value of a string here
 *   event_emitter.on("event", cb);
 *   event_emitter.emit<void(const std::string &)>("event", "test1");
 *   event_emitter.emit<decltype(cb)>("event", "test2");
 */
class event_emitter {
public:
	typedef std::function<void()> entry_t;
	typedef std::list<entry_t> list_t;
	typedef std::pair<list_t &, list_t::iterator> off_handle_t;


	event_emitter() {
		this->_events.max_load_factor(0.75);
	}

	template<typename T>
	off_handle_t on(size_t id, const T &callback) {
		static_assert(std::is_convertible<entry_t, T>::value, "second argument must be std::function");

		list_t &list = this->_events[id];
		list.emplace_back(reinterpret_cast<const entry_t &>(callback));
		return off_handle_t(list, --list.end());
	}

	template<typename T>
	off_handle_t on(const std::string &id, const T &callback) {
		return this->on<T>(std::hash<std::string>()(id), callback);
	}

	template<typename T, typename... A>
	void emit(size_t id, A&&... args) {
		typedef typename std::conditional<std::is_convertible<entry_t, T>::value, T, std::function<T>>::type callback_t;

		const auto iter = this->_events.find(id);

		if (iter != this->_events.end()) {
			for (const entry_t &cb : iter->second) {
				reinterpret_cast<const callback_t &>(cb)(std::forward<A>(args)...);
			}
		}
	}

	template<typename T, typename... A>
	void emit(const std::string &id, A&&... args) {
		this->emit<T>(std::hash<std::string>()(id), std::forward<A>(args)...);
	}

	void off(off_handle_t &handle) {
		handle.first.erase(handle.second);
	}

private:
	std::unordered_map<size_t, list_t> _events;
};

} // namespace events

#endif // nodecc_events_event_emitter_h
