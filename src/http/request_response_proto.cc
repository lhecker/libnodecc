#include "libnodecc/http/request_response_proto.h"

#include "libnodecc/net/socket.h"


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
	return this->write(&buf, 1, false);
}

bool request_response_proto::write(const node::buffer bufs[], size_t bufcnt) {
	return this->write(bufs, bufcnt, false);
}

bool request_response_proto::end() {
	return this->write(nullptr, 0, true);
}

bool request_response_proto::end(const node::buffer& buf) {
	return this->write(&buf, 1, true);
}

bool request_response_proto::end(const node::buffer bufs[], size_t bufcnt) {
	return this->write(bufs, bufcnt, true);
}

bool request_response_proto::write(const node::buffer bufs[], size_t bufcnt, bool end) {
	bool ret = true;
	size_t compiledBufcnt = 0;
	size_t compiledBufsPos = 0;
	node::buffer* compiledBufs = nullptr;
	node::mutable_buffer buf;

	// if the headers have not been sent until now, determine if it uses the chunked encoding
	if (!this->_headers_sent) {
		if (this->_headers.find("content-length") != this->_headers.end()) {
			this->_is_chunked = false;
		} else {
			const auto p = this->_headers.find("transfer-encoding");

			/*
			 * It's the first an last write call and
			 * no transfer-encoding has benn specified.
			 * ---> Try to set the content-length.
			 */
			if (p != this->_headers.end()) {
				this->_is_chunked = p->second.compare("chunked") == 0;
			} else {
				if (end) {
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
						/*
						 * The loop above finished without an integer overflow.
						 * ---> Send it using the content-length header.
						 */
						this->set_header("content-length", std::to_string(contentLength));
						this->_is_chunked = false;

						goto contentLengthSuccessfullySet;
					}
				}

				/*
				 * Calculating the content-length failed.
				 * ---> Use chunked encoding.
				 */
				this->_headers.emplace_hint(p, "transfer-encoding", "chunked");
				this->_is_chunked = true;
			}
		}

	contentLengthSuccessfullySet:

		/*
		 * If it this write call will also send the headers and
		 * uses chunked encoding we can append the first
		 * chunk length field directly to it.
		 *
		 * If it isn't chunked we must use a seperate buffer for the header.
		 */
		if (!this->_is_chunked) {
			compiledBufcnt = 1;
		}
	}

	/*
	 * If the chunked encoding is used we need 2*n+1 bufs:
	 * ([header] + hex + \r\n) + data + (\r\n + hex + \r\n) + ... + (\r\n + 0\r\n\r\n)
	 */
	if (bufcnt > 0) {
		compiledBufcnt += this->_is_chunked ? (2 * bufcnt + 1) : bufcnt;
	} else if (end) {
		/*
		 * TODO: add better stateful behaviour:
		 * preventing undefined behaviour on multiple calls to .end()
		 */
		compiledBufcnt++;
	}

	if (compiledBufcnt == 0) {
		goto writeEnd;
	} else {
		// no need to copy the buffers if we pipe the bufs 1:1 to the socket
		if (compiledBufcnt == bufcnt) {
			ret = this->socket_write(bufs, bufcnt);
			goto writeEnd;
		}

		compiledBufs = (node::buffer*)alloca(compiledBufcnt * sizeof(node::buffer));
		new(compiledBufs) node::buffer[compiledBufcnt]();
	}

	if (!this->_headers_sent) {
		// an average HTTP header should be between 700-800 byte in size
		buf.reserve(800);
		this->compile_headers(buf);

		/*
		 * If it this write call will also send the headers and
		 * uses chunked encoding we can append the first
		 * chunk length field directly to it.
		 *
		 * If it isn't chunked we must use a seperate buffer for the header.
		 */
		if (!this->_is_chunked) {
			compiledBufs[compiledBufsPos++] = buf;
		}

		this->_headers.clear();
		this->_headers_sent = true;
	}

	if (this->_is_chunked) {
		for (size_t i = 0; i < bufcnt; i++) {
			const size_t size = bufs[i].size();

			if (i > 0) {
				buf.append("\r\n");
			}

			buf.append_number(size, 16);
			buf.append("\r\n");

			compiledBufs[compiledBufsPos++] = buf;
			compiledBufs[compiledBufsPos++] = bufs[i];

			buf.reset();
		}

		static const node::buffer normalChunk = node::buffer("\r\n", node::weak);
		static const node::buffer endChunk = node::buffer("\r\n0\r\n\r\n", node::weak);
		compiledBufs[compiledBufsPos++] = end ? endChunk : normalChunk;
	} else {
		for (size_t i = 0; i < bufcnt; i++) {
			compiledBufs[compiledBufsPos++] = bufs[i];
		}
	}

	ret = this->socket_write(compiledBufs, compiledBufsPos);


writeEnd:

	for (size_t i = 0; i < compiledBufsPos; i++) {
		compiledBufs[i].~buffer();
	}

	if (end) {
		this->_headers_sent = false;
	}

	return ret;
}

} // namespace node
} // namespace http
