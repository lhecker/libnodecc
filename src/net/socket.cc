#include "libnodecc/net/socket.h"

#include "libnodecc/dns/lookup.h"


namespace {

struct net_socket_connect {
	explicit net_socket_connect(node::net::socket* socket, const std::shared_ptr<addrinfo>& ai) : socket(socket), ai(ai) {
		this->req.data = this;
		this->current_ai = this->ai.get();
	};

	addrinfo* next() {
		this->current_ai = this->current_ai->ai_next;
		return this->current_ai;
	}

	void connect() {
		sockaddr* addr = this->current_ai->ai_addr;
		int err = INT_MAX;

		if (addr->sa_family == AF_INET || addr->sa_family == AF_INET6) {
			err = uv_tcp_connect(&this->req, *this->socket, addr, [](uv_connect_t* req, int status) {
				auto self = reinterpret_cast<net_socket_connect*>(req->data);

				if (status == 0) {
					// connect successful ---> call callback with true
					if (self->socket->on_connect) {
						self->socket->on_connect(true);
					}
				} else {
					// connect NOT successful but another address is available ---> call next connect
					if (self->next()) {
						static_cast<node::loop&>(*self->socket).next_tick(std::bind(&net_socket_connect::connect, self));
						return;
					} else {
						// connect NOT successful and NO another address available ---> call callback with false
						if (self->socket->on_connect) {
							self->socket->on_connect(false);
						}
					}
				}

				// delete this instance if it finished connecting (if it tries again, it will return; above)
				delete self;
			});
		}

		if (err != 0) {
			if (this->next()) {
				static_cast<node::loop&>(*this->socket).next_tick(std::bind(&net_socket_connect::connect, this));
			} else {
				// connect NOT successful and NO another address available ---> call callback with false
				if (this->socket->on_connect) {
					this->socket->on_connect(false);
				}

				delete this;
			}
		}
	}

	node::net::socket* socket;
	addrinfo* current_ai;
	std::shared_ptr<addrinfo> ai;
	uv_connect_t req;
};

}


namespace node {
namespace net {

socket::socket() : uv::stream<uv_tcp_t>() {
}

bool socket::init(node::loop& loop) {
	return 0 == uv_tcp_init(loop, *this);
}

bool socket::connect(const std::string& address, uint16_t port) {
	dns::lookup([this](const std::shared_ptr<addrinfo>& res) {
		if (res) {
			net_socket_connect* data = new net_socket_connect(this, res);
			data->connect();
		}
	}, *this, address, port);

	return true;
}

bool socket::keepalive(unsigned int delay) {
	return 0 == uv_tcp_keepalive(*this, delay > 0 ? 1 : 0, delay);
}

bool socket::nodelay(bool enable) {
	return 0 == uv_tcp_nodelay(*this, enable ? 1 : 0);
}

} // namespace node
} // namespace net
