#include "libnodecc/http/client_request.h"

#include <cassert>


#define M_LOG16 2.7725887222397812376689284858327062723020005374410210 // log(16)


http::client_request::client_request(uv_loop_t *loop) : net::socket(loop), method("GET"), path("/"), hostname("localhost"), _headersSent(false) {
	this->_headers.max_load_factor(0.75);
}

void http::client_request::sendHeaders() {
	if (!this->_headersSent) {
		this->read_start();

		this->_headersSent = true;
		this->_isChunked = this->_headers.find("content-length") == this->_headers.end();

		std::string buf;

		{
			buf.append(this->method);
			buf.push_back(' ');
			buf.append(this->path);
			buf.append(" HTTP/1.1\r\nhost: ", 17);
			buf.append(this->hostname);
			buf.append("\r\n", 2);
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

		net::socket::write(buf);
		this->_headers.clear();
	}
}

void http::client_request::write(const std::string &str) {
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
				*--to = hex_lookup[length & 15];
			} while (length >>= 4);
		}

		chunkedStr.append("\r\n", 2);
		chunkedStr.append(str);
		chunkedStr.append("\r\n", 2);

		net::socket::write(chunkedStr);
	} else {
		net::socket::write(str);
	}
}

void http::client_request::end() {
	this->sendHeaders();

	if (this->_isChunked) {
		net::socket::write("0\r\n\r\n");
	}

	this->_headersSent = false;
}
