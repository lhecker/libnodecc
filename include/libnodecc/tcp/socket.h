#ifndef nodecc_tcp_socket_h
#define nodecc_tcp_socket_h

#include "../uv/stream.h"


namespace node {
namespace tcp {

class socket : public node::uv::stream<uv_tcp_t> {
	friend struct connect_pack;

public:
	typedef std::function<void(const std::error_code* err, const node::shared_ptr<node::tcp::socket>& socket)> dns_connect_t;

	static const node::events::type<void()> connect_event;

	static void connect(node::loop& loop, const node::string& address, uint16_t port, dns_connect_t cb);

	explicit socket(node::loop& loop);
	void connect(const sockaddr& addr);
	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);

protected:
	~socket() override = default;

private:
	static void _try_next();
};

} // namespace tcp
} // namespace node

#endif // nodecc_tcp_socket_h
