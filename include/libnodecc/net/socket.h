#ifndef nodecc_net_socket_h
#define nodecc_net_socket_h

#include "../uv/stream.h"


namespace node {
namespace net {

class socket : public node::uv::stream<uv_tcp_t> {
	NODE_ADD_CALLBACK(connect, void, bool ok)

public:
	explicit socket();

	bool init(node::loop& loop);

	bool connect(const std::string& address, uint16_t port);

	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);

private:
	friend struct net_socket_connect;
};

} // namespace net
} // namespace node

#endif // nodecc_net_socket_h
