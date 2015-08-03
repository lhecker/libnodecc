#include "libnodecc/net/server.h"


namespace node {
namespace net {

server::server() : uv::handle<uv_tcp_t>() {
}

bool server::init(node::loop& loop) {
	return 0 == uv_tcp_init(loop, *this);
}

bool server::listen(const sockaddr& addr, int backlog, bool dualstack) {
	if (uv_tcp_bind(*this, &addr, dualstack ? 0 : UV_TCP_IPV6ONLY) != 0) {
		return false;
	}

	return 0 == uv_listen(reinterpret_cast<uv_stream_t*>(&this->_handle), backlog, [](uv_stream_t* server, int status) {
		if (status == 0) {
			auto self = reinterpret_cast<node::net::server*>(server->data);
			self->connection_callback.emit();
		}
	});
}

bool server::listen4(uint16_t port, const std::string& ip, int backlog) {
	sockaddr_in addr;
	uv_ip4_addr(ip.c_str(), port, &addr);
	return this->listen(reinterpret_cast<const sockaddr&>(addr), backlog);
}

bool server::listen6(uint16_t port, const std::string& ip, int backlog, bool dualstack) {
	sockaddr_in6 addr;
	uv_ip6_addr(ip.c_str(), port, &addr);
	return this->listen(reinterpret_cast<const sockaddr&>(addr), backlog);
}

bool server::accept(socket& client) {
	return client.init(*this) && 0 == uv_accept(reinterpret_cast<uv_stream_t*>(&this->_handle), static_cast<uv_stream_t*>(client));
}

uint16_t server::port() {
	sockaddr_in name;
	int nameLen = sizeof(name);

	if (0 == uv_tcp_getsockname(*this, (sockaddr*)&name, &nameLen)) {
		return ntohs(name.sin_port);
	} else {
		return 0;
	}
}

} // namespace node
} // namespace net
