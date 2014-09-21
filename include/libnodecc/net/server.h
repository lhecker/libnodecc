#ifndef nodecc_net_server_h
#define nodecc_net_server_h

#include "socket.h"


namespace node {
namespace net {

class server : public node::uv::stream<uv_tcp_t> {
	NODE_ADD_CALLBACK(connection)

public:
	explicit server();

	bool init(node::loop& loop);

	bool listen(uint16_t port, const std::string& ip = "0.0.0.0", int backlog = 511);
	bool accept(node::net::socket& client);

	uint16_t port();
};

} // namespace net
} // namespace node

#endif // nodecc_net_server_h
