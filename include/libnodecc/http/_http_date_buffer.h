#ifndef nodecc_http_http_date_buffer_h
#define nodecc_http_http_date_buffer_h

#include <atomic>
#include <mutex>

#include "../buffer.h"


namespace node {
namespace http {

/*
 * Caches the result of time() and gmtime_r() in a buffer as a string.
 *
 * TODO: Stop using the std::atomic on _last_update
 * if it's not lock free on the specific platform
 * (i.e. if ATOMIC_LLONG_LOCK_FREE is 0).
 * In that case the _mutex should be used instead.
 */
#if defined(ATOMIC_LLONG_LOCK_FREE) && ATOMIC_LLONG_LOCK_FREE > 0
# define HTTP_DATE_BUFFER_IS_ATOMIC
#endif

class http_date_buffer {
public:
	explicit http_date_buffer();

	void update(uint64_t now);
	node::buffer get_buffer();

private:
	void update_buffer();

#ifdef HTTP_DATE_BUFFER_IS_ATOMIC
	std::atomic<uint64_t> _last_update;
#else
	uint64_t _last_update;
#endif

	std::mutex _mutex;
	node::buffer _buffer;
};

} // namespace http
} // namespace node

#endif // nodecc_http_http_date_buffer_h
