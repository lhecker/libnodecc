#include "libnodecc/http/_http_date_buffer.h"


#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 199506L
#endif
#include <ctime>
#ifdef __STDC_WANT_SECURE_LIB__
# define gmtime_r(timep, result) gmtime_s(result, timep)
#endif


#define UPDATE_TIMEOUT 500 // ms


namespace node {
namespace http {


/*
 * Regarding the use of "#if HTTP_DATE_BUFFER_IS_ATOMIC":
 *   If no atomics are provided the mutex will be locked manually at both
 *   points where the update_buffer() function gets invoked
 *   (in the constructor and in the update() method).
 *
 *   Reason being that the update() method needs to check the
 *   non-atomic _last_update member and choose a thread to update
 *   the buffer which requires a full synchronization without atomics.
 */

http_date_buffer::http_date_buffer() : _last_update(0) {
#ifndef HTTP_DATE_BUFFER_IS_ATOMIC
	std::lock_guard<std::mutex> lock(this->_mutex);
#endif

	this->update_buffer();
}

void http_date_buffer::update(uint64_t now) {
#ifdef HTTP_DATE_BUFFER_IS_ATOMIC
	uint64_t last_update = this->_last_update.load(std::memory_order_acquire);

	// first check if the date is stale at all
	if (now - last_update > UPDATE_TIMEOUT) {

		// if we are stale choose ONE thread to call update_buffer()
		if (this->_last_update.compare_exchange_strong(last_update, now, std::memory_order_release, std::memory_order_relaxed)) {
			this->update_buffer();
		}
	}
#else
	std::lock_guard<std::mutex> lock(this->_mutex);

	if (now - this->_last_update >= 500) {
		this->update_buffer();
	}
#endif
}

node::buffer http_date_buffer::get_buffer() {
	std::lock_guard<std::mutex> lock(this->_mutex);

	return this->_buffer;
}

void http_date_buffer::update_buffer() {
	static const uint8_t wday[7][3] = {
		{ 'S', 'u', 'n' },
		{ 'M', 'o', 'n' },
		{ 'T', 'u', 'e' },
		{ 'W', 'e', 'd' },
		{ 'T', 'h', 'u' },
		{ 'F', 'r', 'i' },
		{ 'S', 'a', 't' },
	};

	static const uint8_t mon[12][3] = {
		{ 'J', 'a', 'n' },
		{ 'F', 'e', 'b' },
		{ 'M', 'a', 'r' },
		{ 'A', 'p', 'r' },
		{ 'M', 'a', 'y' },
		{ 'J', 'u', 'n' },
		{ 'J', 'u', 'l' },
		{ 'A', 'u', 'g' },
		{ 'S', 'e', 'p' },
		{ 'O', 'c', 't' },
		{ 'N', 'o', 'v' },
		{ 'D', 'e', 'c' },
	};

	tm t;
	time_t ts = time(nullptr);
	gmtime_r(&ts, &t);

	node::buffer buffer(37);

	if (buffer) {
		char* data = buffer.data<char>();

#define to_char(x) ('0' + (x))

		data[0]  = 'd';
		data[1]  = 'a';
		data[2]  = 't';
		data[3]  = 'e';
		data[4]  = ':';
		data[5]  = ' ';

		// %a Tue
		const auto tm_wday = t.tm_wday;
		data[6]  = wday[tm_wday][0];
		data[7]  = wday[tm_wday][1];
		data[8]  = wday[tm_wday][2];

		data[9]  = ',';
		data[10] = ' ';

		// %d 15
		const auto tm_mday = t.tm_mday;
		data[11] = to_char(tm_mday / 10);
		data[12] = to_char(tm_mday % 10);

		data[13] = ' ';

		// %b Nov
		const auto tm_mon = t.tm_mon;
		data[14]  = mon[tm_mon][0];
		data[15]  = mon[tm_mon][1];
		data[16]  = mon[tm_mon][2];

		data[17] = ' ';

		// %Y 1994
		const auto tm_year = t.tm_year + 1900;
		data[18] = to_char((tm_year / 1000)     );
		data[19] = to_char((tm_year /  100) % 10);
		data[20] = to_char((tm_year /   10) % 10);
		data[21] = to_char((tm_year       ) % 10);

		data[22] = ' ';

		// %H 12
		const auto tm_hour = t.tm_hour;
		data[23] = to_char(tm_hour / 10);
		data[24] = to_char(tm_hour % 10);

		data[25] = ':';

		// %M 45
		const auto tm_min = t.tm_min;
		data[26] = to_char(tm_min / 10);
		data[27] = to_char(tm_min % 10);

		data[28] = ':';

		// %S 26
		const auto tm_sec = t.tm_sec;
		data[29] = to_char(tm_sec / 10);
		data[30] = to_char(tm_sec % 10);

		data[31] = ' ';
		data[32] = 'G';
		data[33] = 'M';
		data[34] = 'T';
		data[35] = '\r';
		data[36] = '\n';

#undef to_char
	}

#ifdef HTTP_DATE_BUFFER_IS_ATOMIC
	std::lock_guard<std::mutex> lock(this->_mutex);
#endif

	this->_buffer = std::move(buffer);
}

} // namespace http
} // namespace node
