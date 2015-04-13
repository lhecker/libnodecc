#include "libnodecc/http/server_response.h"

#include <atomic>
#include <ctime>
#include <mutex>
#include <iostream>

#include "libnodecc/net/socket.h"


/*
 * Caches the result of time() and gmtime_r() in a buffer as a string.
 *
 * TODO: Stop using the std::atomic on _last_update
 * if it's not lock free on the specific platform
 * (i.e. if ATOMIC_LLONG_LOCK_FREE is 0).
 * In that case the _mutex should be used instead.
 */
class date_buffer_t {
public:
	date_buffer_t() {
		uv_rwlock_init(&this->_mutex);
		this->update_buffer();
	}

	~date_buffer_t() {
		uv_rwlock_destroy(&this->_mutex);
	}

	void update(uint64_t now) {
		uint64_t last_update = this->_last_update.load(std::memory_order_relaxed);

		if (now - last_update >= 500) {
			if (this->_last_update.compare_exchange_strong(last_update, now)) {
				uv_rwlock_wrlock(&this->_mutex);
				this->update_buffer();
				uv_rwlock_wrunlock(&this->_mutex);
			}
		}
	}

	node::buffer get_buffer() {
		uv_rwlock_rdlock(&this->_mutex);
		node::buffer buffer(this->_buffer);
		uv_rwlock_rdunlock(&this->_mutex);

		return buffer;
	}

private:
	void update_buffer() {
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

		this->_buffer = std::move(buffer);
	}

	std::atomic<uint64_t> _last_update = {0};
	node::buffer _buffer;
	uv_rwlock_t _mutex;
};


static date_buffer_t date_buffer;


static inline uv_buf_t str_status_code(uint16_t status_code) {
	uv_buf_t ret;

	/*
	 * Most (good) compilers will layout this long case/switch
	 * in a nested one (e.g. first switching 1xx/2xx/3xx/4xx/5xx
	 * and then switching between the lower numbers).
	 *
	 * TODO: Manually optimize for better predictability.
	 */
	switch (status_code) {
	case 100: ret.len = 12; ret.base = const_cast<char*>("100 Continue");                        break;
	case 101: ret.len = 23; ret.base = const_cast<char*>("101 Switching Protocols");             break;
	case 102: ret.len = 14; ret.base = const_cast<char*>("102 Processing");                      break; // RFC 2518, obsoleted by RFC 4918

	case 200: ret.len = 6;  ret.base = const_cast<char*>("200 OK");                              break;
	case 201: ret.len = 11; ret.base = const_cast<char*>("201 Created");                         break;
	case 202: ret.len = 12; ret.base = const_cast<char*>("202 Accepted");                        break;
	case 203: ret.len = 33; ret.base = const_cast<char*>("203 Non-Authoritative Information");   break;
	case 204: ret.len = 14; ret.base = const_cast<char*>("204 No Content");                      break;
	case 205: ret.len = 17; ret.base = const_cast<char*>("205 Reset Content");                   break;
	case 206: ret.len = 19; ret.base = const_cast<char*>("206 Partial Content");                 break;
	case 207: ret.len = 16; ret.base = const_cast<char*>("207 Multi-Status");                    break; // RFC 4918

	case 300: ret.len = 20; ret.base = const_cast<char*>("300 Multiple Choices");                break;
	case 301: ret.len = 21; ret.base = const_cast<char*>("301 Moved Permanently");               break;
	case 302: ret.len = 21; ret.base = const_cast<char*>("302 Moved Temporarily");               break;
	case 303: ret.len = 13; ret.base = const_cast<char*>("303 See Other");                       break;
	case 304: ret.len = 16; ret.base = const_cast<char*>("304 Not Modified");                    break;
	case 305: ret.len = 13; ret.base = const_cast<char*>("305 Use Proxy");                       break;
	case 306: ret.len = 16; ret.base = const_cast<char*>("306 Switch Proxy");                    break; // obsoleted
	case 307: ret.len = 22; ret.base = const_cast<char*>("307 Temporary Redirect");              break;

	case 400: ret.len = 15; ret.base = const_cast<char*>("400 Bad Request");                     break;
	case 401: ret.len = 16; ret.base = const_cast<char*>("401 Unauthorized");                    break;
	case 402: ret.len = 20; ret.base = const_cast<char*>("402 Payment Required");                break;
	case 403: ret.len = 13; ret.base = const_cast<char*>("403 Forbidden");                       break;
	case 404: ret.len = 13; ret.base = const_cast<char*>("404 Not Found");                       break;
	case 405: ret.len = 22; ret.base = const_cast<char*>("405 Method Not Allowed");              break;
	case 406: ret.len = 18; ret.base = const_cast<char*>("406 Not Acceptable");                  break;
	case 407: ret.len = 33; ret.base = const_cast<char*>("407 Proxy Authentication Required");   break;
	case 408: ret.len = 19; ret.base = const_cast<char*>("408 Request Timeout");                 break;
	case 409: ret.len = 12; ret.base = const_cast<char*>("409 Conflict");                        break;
	case 410: ret.len = 8;  ret.base = const_cast<char*>("410 Gone");                            break;
	case 411: ret.len = 19; ret.base = const_cast<char*>("411 Length Required");                 break;
	case 412: ret.len = 23; ret.base = const_cast<char*>("412 Precondition Failed");             break;
	case 413: ret.len = 28; ret.base = const_cast<char*>("413 Request Entity Too Large");        break;
	case 414: ret.len = 25; ret.base = const_cast<char*>("414 Request-URI Too Large");           break;
	case 415: ret.len = 26; ret.base = const_cast<char*>("415 Unsupported Media Type");          break;
	case 416: ret.len = 35; ret.base = const_cast<char*>("416 Requested Range Not Satisfiable"); break;
	case 417: ret.len = 22; ret.base = const_cast<char*>("417 Expectation Failed");              break;
	case 418: ret.len = 16; ret.base = const_cast<char*>("418 I'm a teapot");                    break; // RFC 2324
	case 419: ret.len = 0;  ret.base = nullptr;                                                  break;
	case 420: ret.len = 0;  ret.base = nullptr;                                                  break;
	case 421: ret.len = 0;  ret.base = nullptr;                                                  break;
	case 422: ret.len = 24; ret.base = const_cast<char*>("422 Unprocessable Entity");            break; // RFC 4918
	case 423: ret.len = 10; ret.base = const_cast<char*>("423 Locked");                          break; // RFC 4918
	case 424: ret.len = 21; ret.base = const_cast<char*>("424 Failed Dependency");               break; // RFC 4918
	case 425: ret.len = 24; ret.base = const_cast<char*>("425 Unordered Collection");            break; // RFC 4918
	case 426: ret.len = 20; ret.base = const_cast<char*>("426 Upgrade Required");                break; // RFC 2817
	case 427: ret.len = 0;  ret.base = nullptr;                                                  break;
	case 428: ret.len = 25; ret.base = const_cast<char*>("428 Precondition Required");           break; // RFC 6585
	case 429: ret.len = 21; ret.base = const_cast<char*>("429 Too Many Requests");               break; // RFC 6585
	case 430: ret.len = 0;  ret.base = nullptr;                                                  break;
	case 431: ret.len = 35; ret.base = const_cast<char*>("431 Request Header Fields Too Large"); break; // RFC 6585

	case 500: ret.len = 25; ret.base = const_cast<char*>("500 Internal Server Error");           break;
	case 501: ret.len = 19; ret.base = const_cast<char*>("501 Not Implemented");                 break;
	case 502: ret.len = 15; ret.base = const_cast<char*>("502 Bad Gateway");                     break;
	case 503: ret.len = 23; ret.base = const_cast<char*>("503 Service Unavailable");             break;
	case 504: ret.len = 20; ret.base = const_cast<char*>("504 Gateway Time-out");                break;
	case 505: ret.len = 30; ret.base = const_cast<char*>("505 HTTP Version Not Supported");      break;
	case 506: ret.len = 27; ret.base = const_cast<char*>("506 Variant Also Negotiates");         break; // RFC 2295
	case 507: ret.len = 24; ret.base = const_cast<char*>("507 Insufficient Storage");            break; // RFC 4918
	case 508: ret.len = 17; ret.base = const_cast<char*>("508 Loop Detected");                   break; // RFC 5842
	case 509: ret.len = 28; ret.base = const_cast<char*>("509 Bandwidth Limit Exceeded");        break;
	case 510: ret.len = 16; ret.base = const_cast<char*>("510 Not Extended");                    break; // RFC 2774
	case 511: ret.len = 35; ret.base = const_cast<char*>("511 Network Authentication Required"); break; // RFC 6585

	default:  ret.len = 0;  ret.base = nullptr;                                                  break;
	}

	return ret;
}


namespace node {
namespace http {

server_response::server_response(net::socket& socket) : request_response_proto(), _socket(socket), _status_code(200), _shutdown_on_end(true) {
}

node::net::socket& server_response::socket() const {
	return this->_socket;
}

uint16_t server_response::status_code() const {
	return this->_status_code;
}

void server_response::set_status_code(uint16_t code) {
	this->_status_code = code;
}

void server_response::compile_headers(node::mutable_buffer& buf) {
	{
		uv_buf_t status = str_status_code(this->status_code());

		if (!status.base) {
			status = str_status_code(500);
		}

		/*
		 * As per RFC 2145 §2.3:
		 *
		 * An HTTP server SHOULD send a response version equal to the highest
		 * version for which the server is at least conditionally compliant, and
		 * whose major version is less than or equal to the one received in the
		 * request.
		 */
		buf.append("HTTP/1.1 ");
		buf.append(status.base, status.len);
		buf.append("\r\n");
	}

	if (this->_headers.find("date") == this->_headers.end()) {
		date_buffer.update(uv_now(this->_socket));
		buf.append(date_buffer.get_buffer());
	}

	{
		for (const auto& iter : this->_headers) {
			buf.append(iter.first);
			buf.append(": ");
			buf.append(iter.second);
			buf.append("\r\n");
		}

		buf.append("\r\n");
	}
}

void server_response::socket_write(const node::buffer bufs[], size_t bufcnt) {
	this->socket().write(bufs, bufcnt);
}

void server_response::_end(const node::buffer chunks[], size_t chunkcnt) {
	request_response_proto::_end(chunks, chunkcnt);

	if (this->_shutdown_on_end) {
		this->socket().end();
	}

	// reset fields for the next response in a keepalive connection
	this->set_status_code(200);
}

} // namespace node
} // namespace http
