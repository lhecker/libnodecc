#ifndef nodecc_util_timer_h
#define nodecc_util_timer_h

#include "../uv/handle.h"


namespace node {
namespace util {

class timer : public node::uv::handle<uv_timer_t> {
public:
	node::event<void()> on_timeout;

public:
	explicit timer();

	bool init(node::loop& loop);

	uint64_t repeat() const;

	/**
	 * Sets the repeat timeout in milliseconds.
	 *
	 * If the timer is currently non-repeating, it will be stopped and
	 * rearmed with the new values.
	 *
	 * If the timer is currently repeating, it's old value will
	 * still be used to schedule the next timeout.
	 *
	 * @param repeat Repeat-timeout in milliseconds.
	 */
	void set_repeat(uint64_t repeat);

	/**
	 * Stops the timer and rearms it with it's repeat value as timeout.
	 */
	bool again();

	bool start(uint64_t timeout, uint64_t repeat);
	bool start(uint64_t timeout, uint64_t repeat, decltype(on_timeout)::function_type cb);

	bool stop();
};

} // namespace util
} // namespace node

#endif // nodecc_util_timer_h
