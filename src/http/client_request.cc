#include "libnodecc/http/client_request.h"

#include <cassert>

#include "libnodecc/net/socket.h"


#define M_LOG16 2.7725887222397812376689284858327062723020005374410210 // log(16)


http::client_request::client_request() : request_response_proto(), method("GET"), path("/"), hostname("localhost") {
	this->_headers.max_load_factor(0.75);
}

void http::client_request::send_headers() {
	if (!this->_headersSent) {
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

		net::socket::write(util::buffer(buf, util::copy));
		this->_headers.clear();
	}
}

bool http::client_request::socket_write(const util::buffer bufs[], size_t bufcnt) {
	return net::socket::write(bufs, bufcnt);
}
