#ifndef nodecc_net_socket_h
#define nodecc_net_socket_h

#include "../uv/stream.h"


namespace node {
namespace net {

class socket : public node::uv::stream<uv_tcp_t> {
public:
	node::event<void(int err)> on_connect;

public:
	explicit socket();

	bool init(node::loop& loop);

	bool connect(const sockaddr& addr);
	bool connect(const addrinfo& info);
	bool connect(const std::string& address, uint16_t port);

	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);

private:
	friend struct net_socket_connect;
};

} // namespace net
} // namespace node

#endif // nodecc_net_socket_h
