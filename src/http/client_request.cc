#include "libnodecc/http/client_request.h"

#include <http-parser/http_parser.h>

#include "libnodecc/net/socket.h"


namespace node {
namespace http {

client_request::client_request() : request_response_proto(), _incoming_message(_socket, HTTP_RESPONSE), _method("GET"), _path("/") {
	this->_socket.on_connect([this](int err) {
		if (err) {
			this->emit_error_s();
			this->on_error(nullptr);
			this->on_connect(nullptr);
		} else {
			this->_incoming_message._url = this->_path;
			this->_incoming_message._method = this->_method;
			this->_socket.resume();
			this->emit_connect_s(*this, this->_incoming_message);
		}
	});

	this->_socket.on_close([this]() {
		this->_incoming_message._close();
		this->on_error(nullptr);
		this->on_connect(nullptr);
	});

	this->_incoming_message.on_end([this]() {
		this->_socket.end(); // TODO keep-alive
	});
}

bool client_request::init(node::loop& loop, const sockaddr& addr, const std::string& hostname, on_connect_t cb) {
	this->_on_connect = std::move(cb);
	this->_hostname = hostname;

	return this->_socket.init(loop) && this->_socket.connect(addr);
}

bool client_request::init(node::loop& loop, const std::string& url, on_connect_t cb) {
	http_parser_url u;
	http_parser_parse_url(url.data(), url.size(), 0, &u);

	if (!(u.field_set & UF_SCHEMA) || url.compare(u.field_data[UF_SCHEMA].off, u.field_data[UF_SCHEMA].len, "http") != 0) {
		return false;
	}

	if (u.field_set & UF_HOST) {
		// include ":port" if it exists
		size_t count = u.field_set & UF_PORT ? (u.field_data[UF_PORT].off + u.field_data[UF_PORT].len) - u.field_data[UF_HOST].off : u.field_data[UF_HOST].len;
		this->_hostname = url.substr(u.field_data[UF_HOST].off, count);
	} else {
		this->_hostname = "localhost";
	}

	if (u.field_set & UF_PATH) {
		this->_path = url.substr(u.field_data[UF_PATH].off);
	} else {
		this->_path = "/";
	}

	this->_on_connect = std::move(cb);

	// hostname needed for name resolution
	if (!this->_hostname.empty()) {
		return this->_socket.init(loop) && this->_socket.connect(this->_hostname, u.port ? u.port : 80);
	} else {
		return false;
	}
}

void client_request::compile_headers(node::mutable_buffer& buf) {
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
}

bool client_request::socket_write(const node::buffer bufs[], size_t bufcnt) {
	this->_socket.writev(bufs, bufcnt);
	return true;
}

void client_request::close() {
	this->_socket.close();
}

void client_request::shutdown() {
	this->_socket.end();
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

const node::net::socket& client_request::socket() const {
	return this->_socket;
}

} // namespace node
} // namespace http
