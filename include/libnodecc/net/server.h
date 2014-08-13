#ifndef nodecc_net_server_h
#define nodecc_net_server_h

#include "socket.h"


namespace net {

class server : public uv::stream<uv_tcp_t> {
public:
	typedef std::function<void()> on_connection_t;


	explicit server(uv_loop_t *loop);

	bool listen(uint16_t port, const std::string &ip = "0.0.0.0", int backlog = 511);
	bool accept(net::socket &client);


	on_connection_t on_connection;
};

} // namespace net

#endif // nodecc_net_server_h
