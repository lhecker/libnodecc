#include "libnodecc/net/socket.h"

#include "libnodecc/dns/lookup.h"


namespace node {
namespace net {

struct net_socket_connect {
	explicit net_socket_connect(socket* socket, const std::shared_ptr<addrinfo>& ai) : socket(socket), ai(ai) {
		this->req.data = this;
		this->current_ai = this->ai.get();

		this->socket->retain();
	};

	explicit net_socket_connect(socket* socket, const addrinfo& ai) : socket(socket) {
		this->req.data = this;
		this->current_ai = const_cast<addrinfo*>(&ai);
	};

	~net_socket_connect() {
		this->socket->release();
	}

	addrinfo* next() {
		this->current_ai = this->current_ai->ai_next;
		return this->current_ai;
	}

	void connect() {
		sockaddr* addr = this->current_ai->ai_addr;

		int err = !addr ? EINVAL : uv_tcp_connect(&this->req, *this->socket, addr, [](uv_connect_t* req, int status) {
			auto self = reinterpret_cast<net_socket_connect*>(req->data);

			if (status == 0) {
				// connect successful ---> call callback with true
				self->socket->connect_callback.emit(status);
			} else {
				// connect NOT successful but another address is available ---> call next connect
				if (self->next()) {
					self->connect();
					return;
				} else {
					// connect NOT successful and NO another address available ---> call callback with false
					self->socket->connect_callback.emit(status);
					self->socket->connect_callback.connect(nullptr);
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
				this->socket->connect_callback.emit(err);
				this->socket->connect_callback.connect(nullptr);
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

	const auto r = uv_tcp_connect(req, *this, &addr, [](uv_connect_t* req, int status) {
		auto self = reinterpret_cast<net::socket*>(req->data);

		delete req;

		self->connect_callback.emit(status);

		if (status != 0) {
			self->destroy();
		}

		self->release();
	});

	if (r == 0) {
		this->retain();
	}

	return r != 0;
}

bool socket::connect(const addrinfo& info) {
	net_socket_connect* data = new net_socket_connect(this, info);
	data->connect();

	return true;
}

bool socket::connect(const node::string& address, uint16_t port) {
	this->retain();

	dns::lookup([this](int err, const std::shared_ptr<addrinfo>& res) {
		if (err) {
			this->connect_callback.emit(err);
			this->destroy();
		} else {
			// net_socket_connect deletes itself after it's finished
			net_socket_connect* data = new net_socket_connect(this, res);
			data->connect();
		}

		this->release();
	}, *this, address, port);

	return true;
}

bool socket::keepalive(unsigned int delay) {
	return 0 == uv_tcp_keepalive(*this, delay ? 1 : 0, delay);
}

bool socket::nodelay(bool enable) {
	return 0 == uv_tcp_nodelay(*this, enable ? 1 : 0);
}

void socket::_destroy() {
	this->connect_callback.clear();

	node::uv::stream<uv_tcp_t>::_destroy();
}

} // namespace node
} // namespace net
