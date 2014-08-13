#include "libnodecc/net/socket.h"

#include "libnodecc/dns/dns.h"


net::socket::socket(uv_loop_t *loop) : uv::stream<uv_tcp_t>() {
	uv_tcp_init(loop, *this);
}

bool net::socket::connect(const std::string &ip, uint16_t port) {
	dns::lookup(static_cast<uv_handle_t*>(*this)->loop, ip, [this](int err, const addrinfo *res) {
		if (err == 0) {
		}

		uv_connect_t *req = new uv_connect_t;
		req->data = this;

		uv_tcp_connect(req, *this, (const sockaddr *)res->ai_addr, [](uv_connect_t *req, int status) {
			auto self = reinterpret_cast<net::socket*>(req->data);

			if (status == 0 && self->on_connect) {
				self->on_connect();
			}

			delete req;
		});
	});

	return true;
}

bool net::socket::keepalive(unsigned int delay) {
	return 0 == uv_tcp_keepalive(*this, delay > 0 ? 1 : 0, delay);
}

bool net::socket::nodelay(bool enable) {
	return 0 == uv_tcp_nodelay(*this, enable ? 1 : 0);
}
