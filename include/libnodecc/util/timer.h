#ifndef nodecc_util_timer_h
#define nodecc_util_timer_h

#include "../uv/handle.h"


namespace util {

class timer : public uv::handle<uv_timer_t> {
public:
	typedef std::function<void()> on_timeout_t;


	explicit timer();

	bool init(uv::loop& loop);

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
	bool start(uint64_t timeout, uint64_t repeat, on_timeout_t cb);

	bool stop();


	on_timeout_t on_timeout;
};

} // namespace util

#endif // nodecc_util_timer_h
