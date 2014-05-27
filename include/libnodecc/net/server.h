#ifndef node_cpp_net_server_h
#define node_cpp_net_server_h

#include "../uv/handle.h"


namespace net {
	class socket;
}


namespace net {
	class server : public uv::handle<uv_tcp_t> {
	public:
		typedef std::function<void()> on_connection_t;


		explicit server(uv_loop_t *loop);

		bool listen(uint16_t port, const std::string &ip = "0.0.0.0", int backlog = 511);
		bool accept(const net::socket &client);


		on_connection_t on_connection;
	};
}

#endif
