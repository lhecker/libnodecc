#ifndef nodecc_tcp_server_h
#define nodecc_tcp_server_h

#include "socket.h"


namespace node {
namespace tcp {

class server : public node::uv::handle<uv_tcp_t> {
public:
	explicit server();

	bool init(node::loop& loop);

	bool listen(const sockaddr& addr, int backlog = 511, bool dualstack = true);
	bool listen4(uint16_t port = 0, const std::string& ip = "0.0.0.0", int backlog = 511);
	bool listen6(uint16_t port = 0, const std::string& ip = "::", int backlog = 511, bool dualstack = true);

	bool accept(node::tcp::socket& client);

	uint16_t port();

	void _destroy() override;

	node::callback<void()> connection_callback;

protected:
	~server() override = default;
};

} // namespace tcp
} // namespace node

#endif // nodecc_tcp_server_h
