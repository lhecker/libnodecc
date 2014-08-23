#include "libnodecc/http/client_request.h"

#include <cassert>


#define M_LOG16 2.7725887222397812376689284858327062723020005374410210 // log(16)


http::client_request::client_request() : net::socket(), method("GET"), path("/"), hostname("localhost"), _headersSent(false) {
	this->_headers.max_load_factor(0.75);
}

const std::string &http::client_request::getHeader(const std::string &key) {
	return this->_headers.at(key);
}

void http::client_request::setHeader(const std::string &key, const std::string &value) {
	this->_headers.emplace(key, value);
}

bool http::client_request::headersSent() const {
	return this->_headersSent;
}

void http::client_request::sendHeaders() {
	if (!this->_headersSent) {
		this->read_start();

		this->_headersSent = true;
		this->_isChunked = this->_headers.find("content-length") == this->_headers.end();

		std::string buf;
		buf.reserve(1023); // average HTTP header should be between 700-800 byte

		{
			buf.append(this->method);
			buf.push_back(' ');
			buf.append(this->path);
			buf.append(" HTTP/1.1\r\nhost: ");
			buf.append(this->hostname);
			buf.append("\r\n");
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

		net::socket::write(buf);
		this->_headers.clear();
	}
}

void http::client_request::write(const std::string &str) {
	this->sendHeaders();

	if (this->_isChunked) {
		size_t length = str.length();
		assert(length > 0);

		// output length after converting to hex is log() to the base of 16
		unsigned int hexLength = ceil(log(length) / M_LOG16);

		std::string chunkedStr;
		chunkedStr.reserve(hexLength + 2 + length + 2); // +2+2 for those 2 "\r\n" below

		{
			static const char *hex_lookup = "0123456789abcdef";

			do {
				hexLength--;
				chunkedStr.push_back(hex_lookup[(length >> (4 * hexLength)) & 0x0f]);
			} while (hexLength > 0);
		}

		chunkedStr.append("\r\n");
		chunkedStr.append(str);
		chunkedStr.append("\r\n");

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
