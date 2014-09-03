#include "libnodecc/http/request_response_proto.h"

#include <cassert>

#include "libnodecc/net/socket.h"
#include "libnodecc/util/buffer.h"


#define M_LOG16 2.7725887222397812376689284858327062723020005374410210 // log(16)


http::request_response_proto::request_response_proto() : _headersSent(false) {
	this->_headers.max_load_factor(0.75);
}

http::request_response_proto::~request_response_proto() {
}

const std::string &http::request_response_proto::header(const std::string &key) {
	return this->_headers.at(key);
}

void http::request_response_proto::set_header(const std::string &key, const std::string &value) {
	this->_headers.emplace(key, value);
}

bool http::request_response_proto::headers_sent() const {
	return this->_headersSent;
}

bool http::request_response_proto::write(const util::buffer &buf) {
	return this->write(&buf, 1);
}

bool http::request_response_proto::write(const util::buffer bufs[], size_t bufcnt) {
	assert(bufs != nullptr);
	assert(bufcnt > 0);

	this->send_headers();

	if (this->_isChunked) {
		/*
		 * A bufcnt of N results in "2 * N + 1" chunked bufs
		 *
		 * E.g. 2 bufs need to be sent:
		 * (hex + \r\n) + (data) + (\r\n + hex + \r\n) + (data) + (\r\n)
		 */
		size_t chunked_bufcnt = 2 * bufcnt + 1;
		size_t chunked_pos = 0;

		auto chunked_bufs = static_cast<util::buffer*>(alloca(chunked_bufcnt * sizeof(util::buffer)));
		new(chunked_bufs) util::buffer[chunked_bufcnt]();

		std::string chunkedStr;

		for (size_t i = 0; i < bufcnt; i++) {
			size_t size = bufs[i].size();

			// output length after converting to hex is log() to the base of 16
			unsigned int hexLength = ceil(log(size) / M_LOG16);

			chunkedStr.clear();
			chunkedStr.reserve(hexLength + 2 + 2); // +2+2 for those 2 "\r\n" below

			if (i > 0) {
				chunkedStr.append("\r\n");
			}

			{
				static const char *hex_lookup = "0123456789abcdef";

				do {
					hexLength--;
					chunkedStr.push_back(hex_lookup[(size >> (4 * hexLength)) & 0x0f]);
				} while (hexLength > 0);
			}

			chunkedStr.append("\r\n");

			chunked_bufs[chunked_pos++] = util::buffer(chunkedStr, util::copy);
			chunked_bufs[chunked_pos++] = bufs[i];
		}

		chunked_bufs[chunked_pos] = util::buffer("\r\n", util::weak);

		return this->socket_write(chunked_bufs, chunked_bufcnt);
	} else {
		return this->socket_write(bufs, bufcnt);
	}
}

void http::request_response_proto::end() {
	this->send_headers();

	if (this->_isChunked) {
		util::buffer buffer = util::buffer("0\r\n\r\n", util::weak);
		this->socket_write(&buffer, 1);
	}
	
	this->_headersSent = false;
}
