#ifndef libnodecc_common_h
#define libnodecc_common_h

#define NODE_ADD_CALLBACK(name, ...)                            \
	public:                                                     \
		typedef std::function<void(__VA_ARGS__)> on_##name##_t; \
		                                                        \
		template<typename F>                                    \
		void on_##name(F&& f) {                                 \
			this->_on_##name = std::forward<F>(f);              \
		}                                                       \
		                                                        \
	protected:                                                  \
		template<typename... Args>                              \
		bool emit_##name(Args&&... args) {                      \
			if (this->_on_##name) {                             \
				this->_on_##name(std::forward<Args>(args)...);  \
				return true;                                    \
			} else {                                            \
				return false;                                   \
			}                                                   \
		}                                                       \
		                                                        \
	private:                                                    \
		on_##name##_t _on_##name;

#endif // libnodecc_common_h