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

		this->socket = node::make_shared<tcp::socket>(this->loop);
		this->socket->retain();

		sockaddr* addr = this->current_ai->ai_addr;

		const int ret = uv_tcp_connect(&this->req, *this->socket, addr, [](uv_connect_t* req, int status) {
			auto self = reinterpret_cast<connect_pack*>(req->data);

			self->socket->release();

			if (status == 0) {
				// connect successful ---> call callback with socket
				self->cb(nullptr, self->socket);
			} else {
				// connect NOT successful but another address is available ---> call next connect
				if (self->next()) {
					self->connect();
					return;
				} else {
					// connect NOT successful and NO another address available ---> call callback without socket
					auto err = node::uv::to_error(status);
					self->cb(&err, nullptr);
				}
			}

			// delete this instance if it finished connecting (if it tries again, it will return; above)
			delete self;
		});

		if (ret) {
			this->socket->release();

			if (this->next()) {
				// connect NOT successful but another address is available ---> call next connect
				this->connect();
			} else {
				// connect NOT successful and NO another address available ---> call callback without socket
				auto err = node::uv::to_error(ret);
				this->cb(&err, nullptr);
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


socket::socket(node::loop& loop) : uv::stream<uv_tcp_t>() {
	uv_tcp_init(loop, *this);
}

void socket::connect(const sockaddr& addr) {
	auto req = std::unique_ptr<uv_connect_t>(new uv_connect_t);
	req->data = this;

	node::uv::check(uv_tcp_connect(req.get(), *this, &addr, [](uv_connect_t* req, int status) {
		auto self = reinterpret_cast<tcp::socket*>(req->data);

		delete req;

		self->connect_callback.emit(status);

		if (status != 0) {
			self->destroy();
		}

		self->release();
	}));

	this->retain();
	req.release();
}

/*
 * We can't reuse a single socket for this again and again
 * since it's not guaranteed that reusing a socket works.
 * See: http://stackoverflow.com/a/4200797
 *
 * That's why this method must be a factory method.
 */
void socket::connect(node::loop& loop, const node::string& address, uint16_t port, dns_connect_t cb) {
	auto pack = std::unique_ptr<connect_pack>(new connect_pack(loop, std::move(cb)));
	auto pack_ptr = pack.get();

	dns::lookup([pack_ptr](std::error_code* err, const std::shared_ptr<addrinfo>& info) {
		auto pack = std::unique_ptr<connect_pack>(pack_ptr);

		if (err) {
			pack->cb(err, nullptr);
		} else {
			pack->set_ai(info);
			pack->connect();
			pack.release();
		}
	}, loop, address, port);

	pack.release();
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
