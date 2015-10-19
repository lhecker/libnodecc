#ifndef nodecc_tcp_socket_h
#define nodecc_tcp_socket_h

#include "../uv/stream.h"


namespace node {
namespace tcp {

class socket : public node::uv::stream<uv_tcp_t> {
	friend struct connect_pack;

public:
	typedef std::function<void(int err, node::shared_ptr<node::tcp::socket> socket)> dns_connect_t;


	explicit socket();

	bool init(node::loop& loop);

	bool connect(const sockaddr& addr);
	static bool connect(node::loop& loop, const node::string& address, uint16_t port, dns_connect_t cb);

	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);

	void _destroy() override;

	node::callback<void(int err)> connect_callback;

protected:
	~socket() override = default;

private:
	static void _try_next();
};

} // namespace tcp
} // namespace node

#endif // nodecc_tcp_socket_h
