#include "libnodecc/net/socket.h"

#include "libnodecc/dns/dns.h"


namespace {

struct net_socket_connect {
	explicit net_socket_connect(net::socket *socket, const std::shared_ptr<addrinfo> &ai) : ai(ai), socket(socket) {
		this->req.data = this;
		this->current_ai = this->ai.get();
	};

	addrinfo *next() {
		this->current_ai = this->current_ai->ai_next;
		return this->current_ai;
	}

	void connect() {
		uv_tcp_connect(&this->req, *this->socket, this->current_ai->ai_addr, [](uv_connect_t *req, int status) {
			auto self = reinterpret_cast<net_socket_connect*>(req->data);

			if (status == 0) {
				// connect successful ---> call callback with true
				if(self->socket->on_connect) {
					self->socket->on_connect(true);
				}
			} else {
				// connect NOT successful but another address is available ---> call next connect
				if (self->next()) {
					self->connect();
					return;
				} else {
					// connect NOT successful and NO another address available ---> call callback with false
					if(self->socket->on_connect) {
						self->socket->on_connect(false);
					}
				}
			}

			// delete this instance if it finished connecting (if it tries again, it will return; above)
			delete self;
		});
	}

	uv_connect_t req;
	std::shared_ptr<addrinfo> ai;
	addrinfo *current_ai;
	net::socket *socket;
};

}


net::socket::socket() : uv::stream<uv_tcp_t>() {
}

bool net::socket::init(uv_loop_t *loop) {
	return 0 == uv_tcp_init(loop, *this);
}

bool net::socket::connect(const std::string &address, uint16_t port) {
	dns::lookup(*this, address, [this](const std::shared_ptr<addrinfo> &res) {
		uv_connect_t *req = new uv_connect_t;
		req->data = this;

		net_socket_connect *data = new net_socket_connect(this, res);
		data->connect();
	});

	return true;
}

bool net::socket::keepalive(unsigned int delay) {
	return 0 == uv_tcp_keepalive(*this, delay > 0 ? 1 : 0, delay);
}

bool net::socket::nodelay(bool enable) {
	return 0 == uv_tcp_nodelay(*this, enable ? 1 : 0);
}
