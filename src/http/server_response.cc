#include "libnodecc/http/server_response.h"

#include <cassert>
#include <uv.h>

#include "libnodecc/net/socket.h"
#include "libnodecc/util/string.h"


#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 199506L
#endif
#include <ctime>
#ifdef __STDC_WANT_SECURE_LIB__
# define gmtime_r(timep, result) gmtime_s(result, timep)
#endif


uv_buf_t str_status_code(uint16_t status_code) {
	uv_buf_t ret;

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


http::server_response::server_response(net::socket &socket) : request_response_proto(), socket(socket), status_code(200), _shutdown_on_end(false) {
}

void http::server_response::send_headers() {
	this->_is_chunked = !this->_headers.count("content-length");

	util::string buf(800);

	{
		uv_buf_t status = str_status_code(this->status_code);

		if (!status.base) {
			status = str_status_code(500);
		}

		buf.append("HTTP/1.1 ");
		buf.append(status.base, status.len);
		buf.append("\r\n");
	}

	if (!this->_headers.count("date")) {
		char dateBuf[38];

		time_t t = time(NULL);
		tm t2;
		gmtime_r(&t, &t2);
		size_t written = strftime(dateBuf, 38, "date: %a, %d %b %Y %H:%M:%S GMT\r\n", &t2);
		assert(written == 37);

		buf.append(dateBuf, 37);
	}

	if (this->_is_chunked) {
		if (!this->_headers.count("transfer-encoding")) {
			buf.append("transfer-encoding: chunked\r\n");
		}
	}

	{
		for (const auto iter : this->_headers) {
			buf.append(iter.first);
			buf.append(": ");
			buf.append(iter.second);
			buf.append("\r\n");
		}

		buf.append("\r\n");
	}

	util::buffer buffer = buf;
	this->socket_write(&buffer, 1);
	this->_headers.clear();
}

bool http::server_response::socket_write(const util::buffer bufs[], size_t bufcnt) {
	return this->socket.write(bufs, bufcnt);
}

bool http::server_response::end() {
	bool ret = http::request_response_proto::end();

	if (this->_shutdown_on_end) {
		this->socket.shutdown();
	}

	return ret;
}
