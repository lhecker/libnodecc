#ifndef libnodecc_common_h
#define libnodecc_common_h

#define NODE_ADD_CALLBACK(name, ret, ...)                      \
	public:                                                    \
		typedef std::function<ret(__VA_ARGS__)> on_##name##_t; \
		                                                       \
		template<typename F>                                   \
		void on_##name(F&& f) {                                \
			_on_##name = std::forward<F>(f);                   \
		}                                                      \
		                                                       \
	protected:                                                 \
		bool has_##name##_callback() const {                   \
			return static_cast<bool>(_on_##name);              \
		}                                                      \
		                                                       \
		template<typename... Args>                             \
		ret emit_##name(Args&&... args) {                      \
			return _on_##name(std::forward<Args>(args)...);    \
		}                                                      \
		                                                       \
		template<typename... Args>                             \
		bool emit_##name##_s(Args&&... args) {                 \
			if (_on_##name) {                                  \
				_on_##name(std::forward<Args>(args)...);       \
				return true;                                   \
			}                                                  \
			return false;                                      \
		}                                                      \
		                                                       \
	private:                                                   \
		on_##name##_t _on_##name;

#define NODE_ADD_CALLBACK_WITH_MUTEX(name, mutex, ret, ...)    \
	public:                                                    \
		typedef std::function<ret(__VA_ARGS__)> on_##name##_t; \
		                                                       \
		template<typename F>                                   \
		void on_##name(F&& f) {                                \
			std::lock_guard<decltype(mutex)> lock(mutex);      \
			_on_##name = std::forward<F>(f);                   \
		}                                                      \
		                                                       \
	protected:                                                 \
		bool has_##name##_callback() const {                   \
			return static_cast<bool>(_on_##name);              \
		}                                                      \
		                                                       \
		template<typename... Args>                             \
		ret emit_##name(Args&&... args) {                      \
			std::lock_guard<decltype(mutex)> lock(mutex);      \
			return _on_##name(std::forward<Args>(args)...);    \
		}                                                      \
		                                                       \
		template<typename... Args>                             \
		bool emit_##name##_s(Args&&... args) {                 \
			std::lock_guard<decltype(mutex)> lock(mutex);      \
			if (_on_##name) {                                  \
				_on_##name(std::forward<Args>(args)...);       \
				return true;                                   \
			}                                                  \
			return false;                                      \
		}                                                      \
		                                                       \
	private:                                                   \
		on_##name##_t _on_##name;

#define NODE_ADD_CALLBACK_SAFE(name, ...)                            \
	private:                                                         \
		 std::recursive_mutex _##name##_mutex;                       \
	NODE_ADD_CALLBACK_WITH_MUTEX(name, _##name##_mutex, __VA_ARGS__)

#endif // libnodecc_common_h