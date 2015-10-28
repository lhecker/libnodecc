#include "libnodecc/tcp/server.h"


namespace node {
namespace tcp {

decltype(server::connection_event) server::connection_event;


server::server(node::loop& loop) : uv::handle<uv_tcp_t>() {
	node::uv::check(uv_tcp_init(loop, *this));
}

void server::listen(const sockaddr& addr, int backlog, node::tcp::flags flags) {
	node::uv::check(uv_tcp_bind(*this, &addr, static_cast<unsigned int>(flags)));

	node::uv::check(uv_listen(reinterpret_cast<uv_stream_t*>(&this->_handle), backlog, [](uv_stream_t* server, int status) {
		if (status == 0) {
			auto self = reinterpret_cast<node::tcp::server*>(server->data);
			self->emit(connection_event);
		}
	}));
}

void server::listen4(uint16_t port, const node::string& ip, int backlog, node::tcp::flags flags) {
	sockaddr_in addr;
	uv_ip4_addr(ip.c_str(), port, &addr);
	this->listen(reinterpret_cast<const sockaddr&>(addr), backlog, flags);
}

void server::listen6(uint16_t port, const node::string& ip, int backlog, node::tcp::flags flags) {
	sockaddr_in6 addr;
	uv_ip6_addr(ip.c_str(), port, &addr);
	this->listen(reinterpret_cast<const sockaddr&>(addr), backlog, flags);
}

void server::accept(socket& client) {
	node::uv::check(uv_accept(reinterpret_cast<uv_stream_t*>(&this->_handle), static_cast<uv_stream_t*>(client)));
}

void server::address(sockaddr& addr, int& len) {
	node::uv::check(uv_tcp_getsockname(*this, &addr, &len));
}

uint16_t server::port() {
	static_assert(offsetof(sockaddr_in, sin_port) == offsetof(sockaddr_in6, sin6_port), "sockaddr_in and sockaddr_in6 struct layouts must be the same");

	sockaddr_in addr;
	int addrLen = sizeof(addr);

	this->address((sockaddr&)addr, addrLen);

	return ntohs(addr.sin_port);
}

} // namespace node
} // namespace tcp
