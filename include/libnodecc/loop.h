#ifndef nodecc_loop_h
#define nodecc_loop_h

#include <functional>
#include <vector>
#include <uv.h>


namespace node {

class loop {
public:
	typedef std::function<void()> on_tick_t;


	explicit loop() noexcept;
	~loop() noexcept;

	void run();
	bool run_once();
	bool run_nowait();

	void stop();

	bool alive();

	template<typename T>
	void next_tick(T t) {
		this->_tick_callbacks.emplace_back(std::forward<T>(t));
		uv_async_send(&this->_tick_async);
	}


	operator uv_loop_t*();
	operator const uv_loop_t*() const;

protected:
	uv_loop_t _loop;
	uv_async_t _tick_async;
	std::vector<on_tick_t> _tick_callbacks;
};

} // namespace node

#endif // nodecc_loop_h
