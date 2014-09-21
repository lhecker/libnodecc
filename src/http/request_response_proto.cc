#include "libnodecc/http/request_response_proto.h"

#include "libnodecc/net/socket.h"
#include "libnodecc/string.h"


namespace node {
namespace http {

request_response_proto::request_response_proto() : _headers_sent(false) {
	this->_headers.max_load_factor(0.75);
}

request_response_proto::~request_response_proto() {
}

const std::string& request_response_proto::header(const std::string& key) {
	return this->_headers.at(key);
}

void request_response_proto::set_header(const std::string& key, const std::string& value) {
	this->_headers.emplace(key, value);
}

bool request_response_proto::headers_sent() const {
	return this->_headers_sent;
}

bool request_response_proto::write(const node::buffer& buf) {
	return this->write(&buf, 1);
}

bool request_response_proto::write(const node::buffer bufs[], size_t bufcnt) {
	bool ret;

	if (!this->headers_sent()) {
		if (!this->_headers.count("transfer-encoding")) {
			this->set_header("transfer-encoding", "chunked");
		}

		this->send_headers();
		this->_headers_sent = true;
	}

	if (this->_is_chunked) {
		/*
		 * A bufcnt of N results in "2 * N + 1" chunked bufs
		 *
		 * E.g. 2 bufs need to be sent:
		 * (hex + \r\n) + (data) + (\r\n + hex + \r\n) + (data) + (\r\n)
		 */
		size_t chunked_bufcnt = 2 * bufcnt + 1;
		size_t chunked_pos = 0;

		auto chunked_bufs = static_cast<node::buffer*>(alloca(chunked_bufcnt * sizeof(node::buffer)));
		new(chunked_bufs) node::buffer[chunked_bufcnt]();

		node::string chunkedStr;

		for (size_t i = 0; i < bufcnt; i++) {
			size_t size = bufs[i].size();

			// output length after converting to hex is log() to the base of 16
			unsigned int hexLength = std::ceil(std::log(size) / std::log(16));

			chunkedStr.clear();
			chunkedStr.reserve(hexLength + 2 + 2); // +2+2 for those 2 "\r\n" below

			if (i > 0) {
				chunkedStr.append("\r\n");
			}

			{
				static const char* hex_lookup = "0123456789abcdef";

				do {
					hexLength--;
					chunkedStr.push_back(hex_lookup[(size >> (4 * hexLength)) & 0x0f]);
				} while (hexLength > 0);
			}

			chunkedStr.append("\r\n");

			chunked_bufs[chunked_pos++] = chunkedStr;
			chunked_bufs[chunked_pos++] = bufs[i];
		}

		chunked_bufs[chunked_pos] = node::buffer("\r\n", node::weak);

		ret = this->socket_write(chunked_bufs, chunked_bufcnt);

		for (size_t i = 0; i < chunked_bufcnt; i++) {
			chunked_bufs[i].~buffer();
		}
	} else {
		ret = this->socket_write(bufs, bufcnt);
	}

	return ret;
}

bool request_response_proto::end() {
	return this->end(nullptr, 0);
}

bool request_response_proto::end(const node::buffer& buf) {
	return this->end(&buf, 1);
}

bool request_response_proto::end(const node::buffer bufs[], size_t bufcnt) {
	bool ret = true;

	if (!this->headers_sent()) {
		if (bufcnt) {
			size_t contentLength = 0;
			size_t i = 0;

			for (; i < bufcnt; i++) {
				size_t size = bufs[i].size();

				/*
				 * Make sure that we don't cause a integer overflow when summing buffer sizes.
				 * If an overflow happens break this loop and
				 * send the data using the chunked transfer encoding.
				 */
				if (size > SIZE_T_MAX - contentLength) {
					break;
				}

				contentLength += bufs[i].size();
			}

			if (i == bufcnt) {
				// the loop above finished without an integer overflow
				this->set_header("content-length", std::to_string(contentLength));
				this->send_headers();

				ret = this->socket_write(bufs, bufcnt);
			} else {
				// the loop above finished WITH an integer overflow
				// --> send it using the chunked transfer encoding
				this->write(bufs, bufcnt);
			}
		} else {
			// header-only
			this->send_headers();
			this->_headers_sent = false;
			return false;
		}
	}

	if (this->_is_chunked) {
		node::buffer buffer = node::buffer("0\r\n\r\n", node::weak);
		ret = ret && this->socket_write(&buffer, 1);
	}

	this->_headers_sent = false;
	return ret;
}

} // namespace node
} // namespace http
