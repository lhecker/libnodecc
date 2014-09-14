#include "libnodecc/http/client_request.h"

#include "libnodecc/net/socket.h"
#include "libnodecc/util/string.h"


http::client_request::client_request() : request_response_proto(), _incoming_message(_socket, HTTP_RESPONSE), _method("GET"), _path("/") {
	this->_headers.max_load_factor(0.75);

	this->_socket.on_connect = [this](bool ok) {
		if (ok) {
			this->_incoming_message.url = this->_path;
			this->_incoming_message.method = this->_method;
			this->_socket.read_start();
			this->on_connect(*this, this->_incoming_message);
		} else if (this->on_error) {
			this->on_error();
		}
	};

	this->_incoming_message.on_end = [this]() {
		this->_socket.shutdown(); // TODO
	};
}

bool http::client_request::init(uv::loop& loop, const std::string& hostname, const uint16_t port, const on_connect_t& cb) {
	this->on_connect = cb;
	this->_hostname = hostname;
	this->_port = port;

	if (!this->_hostname.empty()) {
		return this->_socket.init(loop) && this->_socket.connect(this->_hostname, this->_port);
	} else {
		return false;
	}
}

void http::client_request::send_headers() {
	this->_is_chunked = !this->_headers.count("content-length");

	util::string buf(800); // average HTTP header should be between 700-800 byte

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

bool http::client_request::socket_write(const util::buffer bufs[], size_t bufcnt) {
	return this->_socket.write(bufs, bufcnt);
}

void http::client_request::close() {
	this->_socket.close();
}

void http::client_request::shutdown() {
	this->_socket.shutdown();
}

const std::string& http::client_request::method() const {
	return this->_method;
}

const std::string& http::client_request::path() const {
	return this->_path;
}

const std::string& http::client_request::hostname() const {
	return this->_hostname;
}

uint16_t http::client_request::port() const {
	return this->_port;
}


void http::client_request::set_method(const std::string& method) {
	this->_method = method;
}

void http::client_request::set_path(const std::string& path) {
	this->_path = path;
}

