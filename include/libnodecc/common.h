#ifndef libnodecc_common_h
#define libnodecc_common_h

#define _NODE_CALLBACK_ADD_TYPE(name, ret, ...)            \
	typedef std::function<ret(__VA_ARGS__)> on_##name##_t;

#define _NODE_CALLBACK_ADD_VARS(name) \
	on_##name##_t _on_##name;



#define _NODE_CALLBACK_ADD_HAS(name)                          \
	bool has_##name##_callback() const {                      \
		return static_cast<bool>(_on_##name);                 \
	}


#define _NODE_CALLBACK_ADD_ON(name)                           \
	template<typename nacF>                                   \
	void on_##name(nacF&& nacf) {                             \
		_on_##name = std::forward<nacF>(nacf);                \
	}

#define _NODE_CALLBACK_ADD_EMIT(name, ret)                    \
	template<typename... nacArgs>                             \
	ret emit_##name(nacArgs&&... nacargs) {                   \
		return _on_##name(std::forward<nacArgs>(nacargs)...); \
	}

#define _NODE_CALLBACK_ADD_EMIT_S(name)                       \
	template<typename... nacArgs>                             \
	bool emit_##name##_s(nacArgs&&... nacargs) {              \
		if (_on_##name) {                                     \
			_on_##name(std::forward<nacArgs>(nacargs)...);    \
			return true;                                      \
		}                                                     \
		return false;                                         \
	}


#define _NODE_CALLBACK_ADD_HAS_WITH_MUTEX(mutex, name)        \
	bool has_##name##_callback() {                            \
		std::lock_guard<decltype(mutex)> lock(mutex);         \
		return static_cast<bool>(_on_##name);                 \
	}


#define _NODE_CALLBACK_ADD_ON_WITH_MUTEX(mutex, name)         \
	template<typename nacF>                                   \
	void on_##name(nacF&& nacf) {                             \
		std::lock_guard<decltype(mutex)> lock(mutex);         \
		_on_##name = std::forward<nacF>(nacf);                \
	}

#define _NODE_CALLBACK_ADD_EMIT_WITH_MUTEX(mutex, name, ret)  \
	template<typename... nacArgs>                             \
	ret emit_##name(nacArgs&&... nacargs) {                   \
		std::lock_guard<decltype(mutex)> lock(mutex);         \
		return _on_##name(std::forward<nacArgs>(nacargs)...); \
	}

#define _NODE_CALLBACK_ADD_EMIT_S_WITH_MUTEX(mutex, name)     \
	template<typename... nacArgs>                             \
	bool emit_##name##_s(nacArgs&&... nacargs) {              \
		std::lock_guard<decltype(mutex)> lock(mutex);         \
		if (_on_##name) {                                     \
			_on_##name(std::forward<nacArgs>(nacargs)...);    \
			return true;                                      \
		}                                                     \
		return false;                                         \
	}


#define NODE_CALLBACK_ADD(vis, name, ret, ...)          \
	vis:                                                \
		_NODE_CALLBACK_ADD_TYPE(name, ret, __VA_ARGS__) \
	private:                                            \
		_NODE_CALLBACK_ADD_VARS(name)                   \
	vis:                                                \
		_NODE_CALLBACK_ADD_HAS(name)                    \
		_NODE_CALLBACK_ADD_ON(name)                     \
		_NODE_CALLBACK_ADD_EMIT(name, ret)              \
		_NODE_CALLBACK_ADD_EMIT_S(name)                 \


#define NODE_CALLBACK_ADD_WITH_MUTEX(mutex, vis, name, ret, ...) \
	vis:                                                         \
		_NODE_CALLBACK_ADD_TYPE(name, ret, __VA_ARGS__)          \
	private:                                                     \
		_NODE_CALLBACK_ADD_VARS(name)                            \
	vis:                                                         \
		_NODE_CALLBACK_ADD_HAS_WITH_MUTEX(mutex, name)           \
		_NODE_CALLBACK_ADD_ON_WITH_MUTEX(mutex, name)            \
		_NODE_CALLBACK_ADD_EMIT_WITH_MUTEX(mutex, name, ret)     \
		_NODE_CALLBACK_ADD_EMIT_S_WITH_MUTEX(mutex, name)


#define NODE_CALLBACK_ADD_SAFE(vis, name, ...)                            \
	private:                                                              \
		std::recursive_mutex _##name##_mutex;                             \
	NODE_CALLBACK_ADD_WITH_MUTEX(_##name##_mutex, vis, name, __VA_ARGS__)

#endif // libnodecc_common_h