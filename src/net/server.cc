#include "libnodecc/net/server.h"


net::server::server() : uv::stream<uv_tcp_t>() {
}

bool net::server::init(uv_loop_t *loop) {
	return 0 == uv_tcp_init(loop, *this);
}

bool net::server::listen(uint16_t port, const std::string &ip, int backlog) {
	sockaddr_in addr;
	uv_ip4_addr(ip.c_str(), port, &addr);

	if (uv_tcp_bind(*this, (const sockaddr *)&addr, 0) != 0) {
		return false;
	}

	return 0 == uv_listen(*this, backlog, [](uv_stream_t *server, int status) {
		auto self = reinterpret_cast<net::server*>(server->data);

		if (self && self->on_connection) {
			self->on_connection();
		}
	});
}

bool net::server::accept(net::socket &client) {
	return 0 == uv_accept(*this, static_cast<uv_stream_t*>(client));
}
