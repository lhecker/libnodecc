#include "libnodecc/udp/socket.h"


namespace node {
namespace udp {

decltype(socket::data_event) socket::data_event;


socket::socket(node::loop& loop) : uv::handle<uv_udp_t>() {
	uv_udp_init(loop, *this);
}

void socket::listen(const sockaddr& addr, node::udp::flags flags) {
	node::uv::check(uv_udp_bind(*this, &addr, static_cast<unsigned int>(flags)));
}

void socket::listen4(uint16_t port, const node::string& ip, node::udp::flags flags) {
	sockaddr_in addr;
	uv_ip4_addr(ip.c_str(), port, &addr);
	this->listen(reinterpret_cast<const sockaddr&>(addr), flags);
}

void socket::listen6(uint16_t port, const node::string& ip, node::udp::flags flags) {
	sockaddr_in6 addr;
	uv_ip6_addr(ip.c_str(), port, &addr);
	this->listen(reinterpret_cast<const sockaddr&>(addr), flags);
}

void socket::address(sockaddr& addr, int& len) {
	node::uv::check(uv_udp_getsockname(*this, &addr, &len));
}

uint16_t socket::port() {
	static_assert(offsetof(sockaddr_in, sin_port) == offsetof(sockaddr_in6, sin6_port), "sockaddr_in and sockaddr_in6 struct layouts must be the same");

	sockaddr_in addr;
	int addrLen = sizeof(addr);

	this->address((sockaddr&)addr, addrLen);

	return ntohs(addr.sin_port);
}

void socket::set_membership(node::udp::membership membership, const node::string& multicast_addr, const node::string& interface_addr) {
	uv_udp_set_membership(*this, multicast_addr.c_str(), interface_addr.c_str(), (uv_membership)membership);
}

void socket::set_multicast_loop(bool on) {
	uv_udp_set_multicast_loop(*this, on);
}

void socket::set_multicast_ttl(int ttl) {
	uv_udp_set_multicast_ttl(*this, ttl);
}

void socket::set_multicast_interface(const node::string& interface_addr) {
	uv_udp_set_multicast_interface(*this, interface_addr.c_str());
}

void socket::set_broadcast(bool on) {
	uv_udp_set_broadcast(*this, on);
}

void socket::set_ttl(int ttl) {
	uv_udp_set_ttl(*this, ttl);
}

void socket::resume() {
	if (this->_is_consuming) {
		return;
	}

	node::uv::check(uv_udp_recv_start(*this, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
		auto self = reinterpret_cast<socket*>(handle->data);

		if (self->_alloc_buffer.use_count() != 1 || self->_alloc_buffer.size() != suggested_size) {
			self->_alloc_buffer.reset(suggested_size);
		}

		buf->base = self->_alloc_buffer.data<char>();
		buf->len = self->_alloc_buffer.size();
	}, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags) {
		auto self = reinterpret_cast<socket*>(handle->data);

		if (nread > 0) {
			const node::buffer buf = self->_alloc_buffer.slice(0, nread);
			self->emit(data_event, *addr, buf);
		} else if (nread < 0) {
			// other error --> hard close
			self->emit(error_event, node::uv::to_error(int(nread)));
			self->destroy();
		}

		self->_alloc_buffer.reset();
	}));

	this->_is_consuming = true;
}

void socket::pause() {
	if (!this->_is_consuming) {
		return;
	}

	node::uv::check(uv_udp_recv_stop(*this));

	this->_is_consuming = false;
}

void socket::write(sockaddr& addr, const node::buffer bufs[], size_t bufcnt) {
	if (bufcnt == 0) {
		return;
	}

	uv_buf_t* uv_bufs = static_cast<uv_buf_t*>(alloca(bufcnt * sizeof(uv_buf_t)));
	size_t total = 0;
	size_t i;

	for (i = 0; i < bufcnt; i++) {
		uv_buf_t* a = uv_bufs + i;
		const node::buffer* b = bufs + i;

		a->base = b->data<char>();
		a->len  = b->size();

		total += b->size();
	}

	const int wi = uv_udp_try_send(*this, uv_bufs, static_cast<unsigned int>(bufcnt), &addr);
	size_t wu = size_t(wi);

	// if uv_try_write() sent all data
	if (wi > 0) {
		if (wu == total) {
			// everything was written
			return;
		}

		// check if for some reason wu contains an erroneous value just in case
		if (wu > total) {
			// -EINVAL
			return;
		}

		// count how many buffers have been fully written...
		for (i = 0; i < bufcnt; i++) {
			size_t len = uv_bufs[i].len;

			if (len > wu) {
				break;
			}

			wu -= len;
		}

		// ...remove them from the lists...
		bufs    += i;
		uv_bufs += i;
		bufcnt  -= i;

		// ...and remove the bytes not fully written from the (now) first buffer
		uv_bufs[0].base += wu;
		uv_bufs[0].len  -= wu;
	}

	struct packed_req {
		explicit packed_req(socket& self, const node::buffer bufs[], size_t bufcnt, size_t total) : ref_list(bufs, bufcnt) {
			this->req.data = &self;
		}

		uv_udp_send_t req;
		node::buffer_ref_list ref_list;
	};

	auto pack = std::unique_ptr<packed_req>(new packed_req(*this, bufs, bufcnt, total));

	node::uv::check(uv_udp_send(&pack->req, *this, uv_bufs, static_cast<unsigned int>(bufcnt), &addr, [](uv_udp_send_t* req, int status) {
		auto self = reinterpret_cast<socket*>(req->data);

		delete reinterpret_cast<packed_req*>(req);

		if (status != 0) {
			self->destroy();
		}

		self->release();
	}));

	this->retain();
	pack.release();
}

} // namespace node
} // namespace udp
