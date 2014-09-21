#ifndef nodecc_net_socket_h
#define nodecc_net_socket_h

#include "../uv/stream.h"


namespace node {
namespace net {

class socket : public node::uv::stream<uv_tcp_t> {
public:
	typedef std::function<void(bool ok)> on_connect_t;


	explicit socket();

	bool init(node::loop& loop);

	bool connect(const std::string& address, uint16_t port);

	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);


	on_connect_t on_connect;
};

} // namespace net
} // namespace node

#endif // nodecc_net_socket_h
