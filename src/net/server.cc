#include "libnodecc/net/server.h"


namespace node {
namespace net {

server::server() : uv::stream<uv_tcp_t>() {
}

bool server::init(node::loop& loop) {
	return 0 == uv_tcp_init(loop, *this);
}

bool server::listen(uint16_t port, const std::string& ip, int backlog) {
	// TODO IPv6
	sockaddr_in addr;
	uv_ip4_addr(ip.c_str(), port, &addr);

	if (uv_tcp_bind(*this, (const sockaddr*)&addr, 0) != 0) {
		return false;
	}

	return 0 == uv_listen(*this, backlog, [](uv_stream_t* server, int status) {
		auto self = reinterpret_cast<node::net::server*>(server->data);
		self->emit_connection();
	});
}

bool server::accept(socket& client) {
	return 0 == uv_accept(*this, static_cast<uv_stream_t*>(client));
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
