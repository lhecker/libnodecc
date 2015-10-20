#ifndef nodecc_tcp_server_h
#define nodecc_tcp_server_h

#include "socket.h"


namespace node {
namespace tcp {

/*
 * TODO:
 *   - Consolidate with node::udp::socket
 *   - remove "dualstack" arg - add "flags" instead
 *   - create common sockaddr_in/sockaddr_in6/sockaddr_storage helpers for address(), listen() and connect()
 */
class server : public node::uv::handle<uv_tcp_t> {
public:
	explicit server(node::loop& loop);

	void listen(const sockaddr& addr, int backlog = 511, bool dualstack = true);
	void listen4(uint16_t port = 0, const node::string& ip = node::literal_string("0.0.0.0", 7), int backlog = 511);
	void listen6(uint16_t port = 0, const node::string& ip = node::literal_string("::", 2),      int backlog = 511, bool dualstack = true);

	void accept(node::tcp::socket& client);

	void address(sockaddr& addr, int& len);
	uint16_t port();

	void _destroy() override;

	node::callback<void()> connection_callback;
};

} // namespace tcp
} // namespace node

#endif // nodecc_tcp_server_h
