#ifndef nodecc_loop_h
#define nodecc_loop_h

#include <functional>
#include <vector>
#include <uv.h>

#include "error.h"


namespace node {

class loop {
public:
	typedef std::function<void()> on_tick_t;


	explicit loop();
	~loop();

	loop(const loop&) = delete;
	loop& operator=(const loop&) = delete;

	void run();
	void run_once();
	void run_nowait();

	void stop();

	bool alive();

	template<typename T>
	void next_tick(T t) {
		this->_tick_callbacks.emplace_back(std::forward<T>(t));
		uv_async_send(&this->_tick_async);
	}


	operator uv_loop_t*();
	operator const uv_loop_t*() const;

public:
	static void _on_async(uv_async_t* handle) noexcept;

	uv_loop_t _loop;

	uv_async_t _tick_async;
	std::vector<on_tick_t> _tick_callbacks;
};

} // namespace node

#endif // nodecc_loop_h
