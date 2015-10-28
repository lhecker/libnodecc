#ifndef nodecc_tcp_server_h
#define nodecc_tcp_server_h

#include "socket.h"


namespace node {
namespace tcp {

enum class flags : unsigned int {
	none      = 0,
	ipv6only  = UV_TCP_IPV6ONLY,
};

constexpr flags operator|(flags a, flags b) {
	return static_cast<flags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

/*
 * TODO:
 *   - Consolidate with node::udp::socket
 *   - remove "dualstack" arg - add "flags" instead
 *   - create common sockaddr_in/sockaddr_in6/sockaddr_storage helpers for address(), listen() and connect()
 */
class server : public node::uv::handle<uv_tcp_t> {
public:
	static const node::events::type<void()> connection_event;

	explicit server(node::loop& loop);

	void listen(const sockaddr& addr, int backlog = 511, node::tcp::flags flags = flags::none);
	void listen4(uint16_t port = 0, const node::string& ip = node::literal_string("0.0.0.0", 7), int backlog = 511, node::tcp::flags flags = flags::none);
	void listen6(uint16_t port = 0, const node::string& ip = node::literal_string("::0", 2), int backlog = 511, node::tcp::flags flags = flags::none);

	void accept(node::tcp::socket& client);

	void address(sockaddr& addr, int& len);
	uint16_t port();

protected:
	~server() override = default;
};

} // namespace tcp
} // namespace node

#endif // nodecc_tcp_server_h
