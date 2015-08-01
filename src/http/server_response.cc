#include "libnodecc/http/server_response.h"

#include "libnodecc/http/_http_date_buffer.h"
#include "libnodecc/net/socket.h"

#include "_status_codes.h"


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
#define XX(_status_, _len_, _str_) case _status_: ret.len = _len_; ret.base = (char*)_str_; break;
	STATUS_CODES(XX)
#undef XX

	default: ret.len = 0; ret.base = nullptr; break;
	}

	return ret;
}


namespace node {
namespace http {

static node::http::http_date_buffer date_buffer;


server_response::server_response(net::socket& socket) : outgoing_message(), _socket(socket), _status_code(200), _shutdown_on_end(true) {
}

node::net::socket& server_response::socket() {
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

	if (this->_headers.find("date"_view) == this->_headers.end()) {
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
	outgoing_message::_end(chunks, chunkcnt);

	if (this->_shutdown_on_end) {
		this->socket().end();
	}

	// reset fields for the next response in a keepalive connection
	this->set_status_code(200);
}

} // namespace node
} // namespace http
