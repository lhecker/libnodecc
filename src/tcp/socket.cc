#include "libnodecc/tcp/socket.h"

#include "libnodecc/dns/lookup.h"


namespace node {
namespace tcp {

struct connect_pack {
	explicit connect_pack(node::loop& loop, socket::dns_connect_t&& cb) : loop(loop), cb(cb) {
		this->req.data = this;
	};

	void set_ai(const std::shared_ptr<addrinfo>& ai) {
		this->ai = ai;
		this->current_ai = this->ai.get();
	}

	addrinfo* next() {
		this->current_ai = this->current_ai->ai_next;
		return this->current_ai;
	}

	void connect() {
		/*
		 * TODO: Is it really not possible to uv_close() and init() again?
		 * But how would one transmit the self pointer across
		 * the close callback without modifying handle.data?
		 */
		this->socket.reset();

		this->socket = node::make_shared<tcp::socket>();
		this->socket->init(this->loop);

		this->socket->retain();

		sockaddr* addr = this->current_ai->ai_addr;

		const int err = uv_tcp_connect(&this->req, *this->socket, addr, [](uv_connect_t* req, int status) {
			auto self = reinterpret_cast<connect_pack*>(req->data);

			self->socket->release();

			if (status == 0) {
				// connect successful ---> call callback with socket
				self->cb(status, self->socket);
			} else {
				// connect NOT successful but another address is available ---> call next connect
				if (self->next()) {
					self->connect();
					return;
				} else {
					// connect NOT successful and NO another address available ---> call callback without socket
					self->cb(status, nullptr);
				}
			}

			// delete this instance if it finished connecting (if it tries again, it will return; above)
			delete self;
		});

		if (err) {
			this->socket->release();

			if (this->next()) {
				// connect NOT successful but another address is available ---> call next connect
				this->connect();
			} else {
				// connect NOT successful and NO another address available ---> call callback without socket
				this->cb(err, nullptr);
				delete this;
			}
		}
	}

	node::loop& loop;
	socket::dns_connect_t cb;
	std::shared_ptr<addrinfo> ai;
	addrinfo* current_ai;
	node::shared_ptr<socket> socket;
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
		auto self = reinterpret_cast<tcp::socket*>(req->data);

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

/*
 * We can't reuse a single socket for this again and again
 * since it's not guaranteed that reusing a socket works.
 * See: http://stackoverflow.com/a/4200797
 *
 * That's why this method must be a factory method.
 */
bool socket::connect(node::loop& loop, const node::string& address, uint16_t port, dns_connect_t cb) {
	connect_pack* pack = new connect_pack(loop, std::move(cb));

	dns::lookup([pack](int err, const std::shared_ptr<addrinfo>& res) {
		if (err) {
			pack->cb(err, nullptr);
			delete pack;
		} else {
			pack->set_ai(res);
			pack->connect();
		}
	}, loop, address, port);

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
} // namespace tcp
