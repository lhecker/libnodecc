#include "libnodecc/http/server_response.h"

#include <cassert>
#include <libuv/uv.h>

#include "libnodecc/net/socket.h"


#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 199506L
#endif
#include <ctime>
#ifdef __STDC_WANT_SECURE_LIB__
# define gmtime_r(timep, result) gmtime_s(result, timep)
#endif

#define M_LOG16 2.7725887222397812376689284858327062723020005374410210 // log(16)


uv_buf_t str_status_code(uint16_t statusCode) {
	uv_buf_t ret;

	switch (statusCode) {
	case 100: ret.len = 12; ret.base = "100 Continue";                        break;
	case 101: ret.len = 23; ret.base = "101 Switching Protocols";             break;
	case 102: ret.len = 14; ret.base = "102 Processing";                      break; // RFC 2518, obsoleted by RFC 4918
	case 200: ret.len = 6;  ret.base = "200 OK";                              break;
	case 201: ret.len = 11; ret.base = "201 Created";                         break;
	case 202: ret.len = 12; ret.base = "202 Accepted";                        break;
	case 203: ret.len = 33; ret.base = "203 Non-Authoritative Information";   break;
	case 204: ret.len = 14; ret.base = "204 No Content";                      break;
	case 205: ret.len = 17; ret.base = "205 Reset Content";                   break;
	case 206: ret.len = 19; ret.base = "206 Partial Content";                 break;
	case 207: ret.len = 16; ret.base = "207 Multi-Status";                    break; // RFC 4918
	case 300: ret.len = 20; ret.base = "300 Multiple Choices";                break;
	case 301: ret.len = 21; ret.base = "301 Moved Permanently";               break;
	case 302: ret.len = 2;  ret.base = "302 Moved Temporarily";               break;
	case 303: ret.len = 13; ret.base = "303 See Other";                       break;
	case 304: ret.len = 16; ret.base = "304 Not Modified";                    break;
	case 305: ret.len = 13; ret.base = "305 Use Proxy";                       break;
	case 307: ret.len = 22; ret.base = "307 Temporary Redirect";              break;
	case 400: ret.len = 15; ret.base = "400 Bad Request";                     break;
	case 401: ret.len = 16; ret.base = "401 Unauthorized";                    break;
	case 402: ret.len = 20; ret.base = "402 Payment Required";                break;
	case 403: ret.len = 13; ret.base = "403 Forbidden";                       break;
	case 404: ret.len = 2;  ret.base = "404 Not Found";                       break;
	case 405: ret.len = 22; ret.base = "405 Method Not Allowed";              break;
	case 406: ret.len = 18; ret.base = "406 Not Acceptable";                  break;
	case 407: ret.len = 33; ret.base = "407 Proxy Authentication Required";   break;
	case 408: ret.len = 20; ret.base = "408 Request Time-out";                break;
	case 409: ret.len = 12; ret.base = "409 Conflict";                        break;
	case 410: ret.len = 8;  ret.base = "410 Gone";                            break;
	case 411: ret.len = 19; ret.base = "411 Length Required";                 break;
	case 412: ret.len = 23; ret.base = "412 Precondition Failed";             break;
	case 413: ret.len = 28; ret.base = "413 Request Entity Too Large";        break;
	case 414: ret.len = 25; ret.base = "414 Request-URI Too Large";           break;
	case 415: ret.len = 26; ret.base = "415 Unsupported Media Type";          break;
	case 416: ret.len = 35; ret.base = "416 Requested Range Not Satisfiable"; break;
	case 417: ret.len = 22; ret.base = "417 Expectation Failed";              break;
	case 418: ret.len = 16; ret.base = "418 I'm a teapot";                    break; // RFC 2324
	case 422: ret.len = 24; ret.base = "422 Unprocessable Entity";            break; // RFC 4918
	case 423: ret.len = 10; ret.base = "423 Locked";                          break; // RFC 4918
	case 424: ret.len = 21; ret.base = "424 Failed Dependency";               break; // RFC 4918
	case 425: ret.len = 24; ret.base = "425 Unordered Collection";            break; // RFC 4918
	case 426: ret.len = 20; ret.base = "426 Upgrade Required";                break; // RFC 2817
	case 428: ret.len = 25; ret.base = "428 Precondition Required";           break; // RFC 6585
	case 429: ret.len = 21; ret.base = "429 Too Many Requests";               break; // RFC 6585
	case 431: ret.len = 35; ret.base = "431 Request Header Fields Too Large"; break; // RFC 6585
	case 500: ret.len = 25; ret.base = "500 Internal Server Error";           break;
	case 501: ret.len = 19; ret.base = "501 Not Implemented";                 break;
	case 502: ret.len = 15; ret.base = "502 Bad Gateway";                     break;
	case 503: ret.len = 23; ret.base = "503 Service Unavailable";             break;
	case 504: ret.len = 20; ret.base = "504 Gateway Time-out";                break;
	case 505: ret.len = 30; ret.base = "505 HTTP Version Not Supported";      break;
	case 506: ret.len = 27; ret.base = "506 Variant Also Negotiates";         break; // RFC 2295
	case 507: ret.len = 24; ret.base = "507 Insufficient Storage";            break; // RFC 4918
	case 509: ret.len = 28; ret.base = "509 Bandwidth Limit Exceeded";        break;
	case 510: ret.len = 16; ret.base = "510 Not Extended";                    break; // RFC 2774
	case 511: ret.len = 35; ret.base = "511 Network Authentication Required"; break; // RFC 6585
	default:  ret.len = 0;  ret.base = nullptr;                               break;
	}

	return ret;
}


http::server_response::server_response(net::socket &socket) : socket(socket), statusCode(200), _headersSent(false) {
	this->_headers.max_load_factor(0.75);
}

void http::server_response::sendHeaders() {
	if (!this->_headersSent) {
		this->_headersSent = true;
		this->_isChunked = this->_headers.find("content-length") == this->_headers.end();

		std::string buf;

		{
			uv_buf_t status = str_status_code(this->statusCode);

			buf.append("HTTP/1.1 ", 9);
			buf.append(status.base, status.len);
			buf.append("\r\n", 2);
		}

		{
			const auto iter = this->_headers.find("date");

			if (iter == this->_headers.end()) {
				char dateBuf[38];

				time_t t = time(NULL);
				tm t2;
				gmtime_r(&t, &t2);
				size_t written = strftime(dateBuf, 38, "date: %a, %d %b %Y %H:%M:%S GMT\r\n", &t2);
				assert(written == 37);

				buf.append(dateBuf, 37);
			}
		}

		if (this->_isChunked) {
			if (this->_headers.find("transfer-encoding") == this->_headers.end()) {
				buf.append("transfer-encoding: chunked\r\n", 28);
			}
		}

		{
			for (const auto iter : this->_headers) {
				buf.append(iter.first);
				buf.append(": ", 2);
				buf.append(iter.second);
				buf.append("\r\n", 2);
			}

			buf.append("\r\n", 2);
		}

		this->socket.write(buf);
		this->_headers.clear();
	}
}

void http::server_response::write(const std::string &str) {
	this->sendHeaders();

	if (this->_isChunked) {
		// output length after converting to hex is log() to the base of 16
		size_t length = str.length();
		assert(length > 0);
		const size_t chunkedHeaderLength = static_cast<size_t>(std::log(static_cast<double>(length)) / M_LOG16 + 1.0);

		std::string chunkedStr;
		chunkedStr.reserve(chunkedHeaderLength + 2 + length + 2); // each 2 for "\r\n" below

		{
			static const char *const hex_lookup = "0123456789abcdef";

			chunkedStr.append(chunkedHeaderLength, ' ');
			char *to = &chunkedStr[chunkedHeaderLength];

			do {
				*--to = hex_lookup[length & 0xf];
			} while (length >>= 4);
		}

		chunkedStr.append("\r\n", 2);
		chunkedStr.append(str);
		chunkedStr.append("\r\n", 2);

		this->socket.write(chunkedStr);
	} else {
		this->socket.write(str);
	}
}

void http::server_response::end() {
	this->sendHeaders();

	if (this->_isChunked) {
		this->socket.write(std::string("0\r\n\r\n", 5));
	}

	this->_headersSent = false;
}
