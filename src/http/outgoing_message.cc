#include "libnodecc/http/outgoing_message.h"

#include "libnodecc/net/socket.h"
#include "libnodecc/util/math.h"


namespace node {
namespace http {

outgoing_message::outgoing_message() : _headers_sent(false) {
	this->_headers.max_load_factor(0.75);
}

outgoing_message::~outgoing_message() {
}

const node::buffer outgoing_message::header(node::buffer_view& key) {
	return this->_headers.at(key);
}

void outgoing_message::set_header(node::buffer key, node::buffer value) {
	this->_headers.emplace(key, value);
}

bool outgoing_message::headers_sent() const {
	return this->_headers_sent;
}

void outgoing_message::_write(const node::buffer chunks[], size_t chunkcnt) {
	this->http_write(chunks, chunkcnt, false);
}

void outgoing_message::_end(const node::buffer chunks[], size_t chunkcnt) {
	this->http_write(chunks, chunkcnt, true);
}

void outgoing_message::http_write(const node::buffer bufs[], size_t bufcnt, bool end) {
	size_t compiledBufcnt = 0;
	size_t compiledBufsPos = 0;
	node::buffer* compiledBufs = nullptr;
	node::mutable_buffer buf;

	// TODO: do not set chunked/content-length headers if there is no body at all (e.g. GET requests)
	// if the headers have not been sent until now, determine if it uses the chunked encoding
	if (!this->_headers_sent) {
		using namespace node::literals;

		if (this->_headers.find("content-length"_buffer_view) != this->_headers.end()) {
			this->_is_chunked = false;
		} else {
			const auto p = this->_headers.find("transfer-encoding"_buffer_view);

			/*
			 * It's the first an last write call and
			 * no transfer-encoding has benn specified.
			 * ---> Try to set the content-length.
			 */
			if (p != this->_headers.end()) {
				this->_is_chunked = p->second.compare("chunked"_buffer_view) == 0;
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
						std::numeric_limits<decltype(contentLength)>::max();
						if (size > 0 - contentLength) {
							break;
						}

						contentLength += bufs[i].size();
					}

					if (i == bufcnt) {
						/*
						 * The loop above finished without an integer overflow.
						 * ---> Send it using the content-length header.
						 */
						node::mutable_buffer buf;
						buf.append_number(contentLength, 10);

						this->set_header("content-length"_buffer_view, buf);
						this->_is_chunked = false;

						goto contentLengthSuccessfullySet;
					}
				}

				/*
				 * Calculating the content-length failed.
				 * ---> Use chunked encoding.
				 */
				this->_headers.emplace_hint(p, "transfer-encoding"_buffer_view, "chunked"_buffer_view);
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
			this->socket_write(bufs, bufcnt);
			goto writeEnd;
		}

		const size_t size = compiledBufcnt * sizeof(node::buffer);

		compiledBufs = (node::buffer*)alloca(size);

		/*
		 * Using the array version of placement-new is only
		 * guaranteed to be compatible with new[] and
		 * might return a pointer different from the passed one.
		 * E.g. MSVS sets the first 4 (or 8 on x64) Bytes to the passed array length.
		 * memset() is fine in our case since node::buffer's default
		 * constructor is a constexpr, setting all members to zero.
		 */
		memset(compiledBufs, 0, size);
	}

	if (!this->_headers_sent) {
		// an average HTTP header should be between 700-800 byte in size
		buf.set_capacity(800);
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

		static const node::buffer normalChunk("\r\n", node::weak);
		static const node::buffer endChunk("\r\n0\r\n\r\n", node::weak);
		compiledBufs[compiledBufsPos++] = end ? endChunk : normalChunk;
	} else {
		for (size_t i = 0; i < bufcnt; i++) {
			compiledBufs[compiledBufsPos++] = bufs[i];
		}
	}

	this->socket_write(compiledBufs, compiledBufsPos);


writeEnd:

	for (size_t i = 0; i < compiledBufsPos; i++) {
		compiledBufs[i].~buffer();
	}

	if (end) {
		this->_headers_sent = false;
	}
}

} // namespace node
} // namespace http