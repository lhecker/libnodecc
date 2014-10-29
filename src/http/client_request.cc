#include "libnodecc/http/client_request.h"

#include "libnodecc/net/socket.h"


namespace node {
namespace http {

client_request::client_request() : request_response_proto(), _incoming_message(_socket, HTTP_RESPONSE), _method("GET"), _path("/") {
	this->_headers.max_load_factor(0.75);

	this->_socket.on_connect([this](int err) {
		if (err) {
			this->emit_error_s();
			this->on_error(nullptr);
			this->on_connect(nullptr);
		} else {
			this->_incoming_message.url = this->_path;
			this->_incoming_message.method = this->_method;
			this->_socket.read_start();
			this->emit_connect_s(*this, this->_incoming_message);
		}
	});

	this->_socket.on_close([this]() {
		this->_incoming_message._close();
		this->on_error(nullptr);
		this->on_connect(nullptr);
	});

	this->_incoming_message.on_end([this]() {
		this->_socket.shutdown(); // TODO keep-alive
	});
}

bool client_request::init(node::loop& loop, const sockaddr& addr, const std::string& hostname, on_connect_t cb) {
	this->_on_connect = std::move(cb);
	this->_hostname = hostname;

	return this->_socket.init(loop) && this->_socket.connect(addr);
}

bool client_request::init(node::loop& loop, const std::string& hostname, const uint16_t port, on_connect_t cb) {
	this->_on_connect = std::move(cb);
	this->_hostname = hostname; // TODO: this should be hostname:port or [hostname]:port (IPv6)

	// hostname needed for name resolution
	if (!this->_hostname.empty()) {
		return this->_socket.init(loop) && this->_socket.connect(this->_hostname, port);
	} else {
		return false;
	}
}

void client_request::send_headers() {
	this->_is_chunked = !this->_headers.count("content-length");

	node::mutable_buffer buf(800); // average HTTP header should be between 700-800 byte

	{
		buf.append(this->_method);
		buf.push_back(' ');
		buf.append(this->_path);
		buf.append(" HTTP/1.1\r\nhost: ");
		buf.append(this->_hostname);
		buf.append("\r\n");
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

	this->_socket.write(buf);
	this->_headers.clear();
}

bool client_request::socket_write(const node::buffer bufs[], size_t bufcnt) {
	return this->_socket.write(bufs, bufcnt);
}

void client_request::close() {
	this->_socket.close();
}

void client_request::shutdown() {
	this->_socket.shutdown();
}

const std::string& client_request::method() const {
	return this->_method;
}

const std::string& client_request::path() const {
	return this->_path;
}

const std::string& client_request::hostname() const {
	return this->_hostname;
}

void client_request::set_method(const std::string& method) {
	this->_method = method;
}

void client_request::set_path(const std::string& path) {
	this->_path = path;
}

} // namespace node
} // namespace http
