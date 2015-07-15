#include "libnodecc/net/socket.h"

#include "libnodecc/dns/lookup.h"



namespace node {
namespace net {

struct net_socket_connect {
	explicit net_socket_connect(socket* socket, const std::shared_ptr<addrinfo>& ai) : socket(socket), ai(ai) {
		this->req.data = this;
		this->current_ai = this->ai.get();
	};

	explicit net_socket_connect(socket* socket, const addrinfo& ai) : socket(socket) {
		this->req.data = this;
		this->current_ai = const_cast<addrinfo*>(&ai);
	};

	addrinfo* next() {
		this->current_ai = this->current_ai->ai_next;
		return this->current_ai;
	}

	void connect() {
		sockaddr* addr = this->current_ai->ai_addr;

		int err = uv_tcp_connect(&this->req, *this->socket, addr, [](uv_connect_t* req, int status) {
			auto self = reinterpret_cast<net_socket_connect*>(req->data);

			if (status == 0) {
				// connect successful ---> call callback with true
				self->socket->on_connect.emit(status);
			} else {
				// connect NOT successful but another address is available ---> call next connect
				if (self->next()) {
					self->connect();
					return;
				} else {
					// connect NOT successful and NO another address available ---> call callback with false
					self->socket->on_connect.emit(status);
					self->socket->on_connect(nullptr);
				}
			}

			// delete this instance if it finished connecting (if it tries again, it will return; above)
			delete self;
		});

		if (err) {
			if (this->next()) {
				this->connect();
			} else {
				// connect NOT successful and NO another address available ---> call callback with false
				this->socket->on_connect.emit(err);
				this->socket->on_connect(nullptr);
				delete this;
			}
		}
	}

	socket* socket;
	addrinfo* current_ai;
	std::shared_ptr<addrinfo> ai;
	uv_connect_t req;
};


socket::socket() : uv::stream<uv_tcp_t>() {
}

bool socket::init(node::loop& loop) {
	return 0 == uv_tcp_init(loop, *this);
}

bool socket::connect(const sockaddr& addr) {
	uv_connect_t* req = new uv_connect_t;
	req->data = this;

	return 0 != uv_tcp_connect(req, *this, &addr, [](uv_connect_t* req, int status) {
		auto self = reinterpret_cast<net::socket*>(req->data);
		delete req;

		self->on_connect.emit(status);

		if (status) {
			self->on_connect(nullptr);
		}
	});
}

bool socket::connect(const addrinfo& info) {
	net_socket_connect* data = new net_socket_connect(this, info);
	data->connect();

	return true;
}

bool socket::connect(const std::string& address, uint16_t port) {
	dns::lookup([this](int err, const std::shared_ptr<addrinfo>& res) {
		if (err) {
			this->on_connect.emit(err);
			this->on_connect(nullptr);
		} else {
			net_socket_connect* data = new net_socket_connect(this, res);
			data->connect();
		}
	}, *this, address, port);

	return true;
}

bool socket::keepalive(unsigned int delay) {
	return 0 == uv_tcp_keepalive(*this, delay ? 1 : 0, delay);
}

bool socket::nodelay(bool enable) {
	return 0 == uv_tcp_nodelay(*this, enable ? 1 : 0);
}

} // namespace node
} // namespace net
