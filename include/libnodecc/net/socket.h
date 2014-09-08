#ifndef nodecc_net_socket_h
#define nodecc_net_socket_h

#include "../uv/stream.h"

#include <cmath>


namespace net {

class socket : public uv::stream<uv_tcp_t> {
public:
	typedef std::function<void(bool ok)> on_connect_t;


	explicit socket();

	bool init(uv::loop& loop);

	bool connect(const std::string& address, uint16_t port);

	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);


	on_connect_t on_connect;
};

} // namespace net

#endif // nodecc_net_socket_h
