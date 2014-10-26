#ifndef libnodecc_common_h
#define libnodecc_common_h

#define NODE_ADD_CALLBACK(vis, name, ret, ...)                    \
	vis:                                                          \
		typedef std::function<ret(__VA_ARGS__)> on_##name##_t;    \
		                                                          \
	private:                                                      \
		on_##name##_t _on_##name;                                 \
		                                                          \
	protected:                                                    \
		template<typename... nacArgs>                             \
		ret emit_##name(nacArgs&&... nacargs) {                   \
			return _on_##name(std::forward<nacArgs>(nacargs)...); \
		}                                                         \
		                                                          \
		template<typename... nacArgs>                             \
		bool emit_##name##_s(nacArgs&&... nacargs) {              \
			if (_on_##name) {                                     \
				_on_##name(std::forward<nacArgs>(nacargs)...);    \
				return true;                                      \
			}                                                     \
			return false;                                         \
		}                                                         \
		                                                          \
	vis:                                                          \
		template<typename nacF>                                   \
		void on_##name(nacF&& nacf) {                             \
			_on_##name = std::forward<nacF>(nacf);                \
		}                                                         \
		                                                          \
		bool has_##name##_callback() const {                      \
			return static_cast<bool>(_on_##name);                 \
		}

#define NODE_ADD_CALLBACK_WITH_MUTEX(vis, name, mutex, ret, ...)  \
	vis:                                                          \
		typedef std::function<ret(__VA_ARGS__)> on_##name##_t;    \
		                                                          \
	private:                                                      \
		on_##name##_t _on_##name;                                 \
		                                                          \
	protected:                                                    \
		template<typename... nacArgs>                             \
		ret emit_##name(nacArgs&&... nacargs) {                   \
			std::lock_guard<decltype(mutex)> lock(mutex);         \
			return _on_##name(std::forward<nacArgs>(nacargs)...); \
		}                                                         \
		                                                          \
		template<typename... nacArgs>                             \
		bool emit_##name##_s(nacArgs&&... nacargs) {              \
			std::lock_guard<decltype(mutex)> lock(mutex);         \
			if (_on_##name) {                                     \
				_on_##name(std::forward<nacArgs>(nacargs)...);    \
				return true;                                      \
			}                                                     \
			return false;                                         \
		}                                                         \
		                                                          \
	vis:                                                          \
		template<typename nacF>                                   \
		void on_##name(nacF&& nacf) {                             \
			std::lock_guard<decltype(mutex)> lock(mutex);         \
			_on_##name = std::forward<nacF>(nacf);                \
		}                                                         \
		                                                          \
		bool has_##name##_callback() const {                      \
			return static_cast<bool>(_on_##name);                 \
		}


#define NODE_ADD_CALLBACK_SAFE(vis, name, ...)                            \
	private:                                                              \
		std::recursive_mutex _##name##_mutex;                             \
	NODE_ADD_CALLBACK_WITH_MUTEX(vis, name, _##name##_mutex, __VA_ARGS__)

#endif // libnodecc_common_h