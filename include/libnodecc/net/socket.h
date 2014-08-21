#ifndef nodecc_net_socket_h
#define nodecc_net_socket_h

#include <cmath>

#include "../uv/stream.h"


namespace net {

class socket : public uv::stream<uv_tcp_t> {
public:
	typedef std::function<void()> on_connect_t;


	explicit socket();

	bool init(uv_loop_t *loop);

	bool connect(const std::string &ip, uint16_t port);

	bool keepalive(unsigned int delay);
	bool nodelay(bool enable);


	on_connect_t on_connect;
};

} // namespace net


namespace std {

template<>
struct hash<net::socket> {
	size_t operator()(const net::socket &val) const {
		/*
		 * Instances are likely to be aligned along the size of the class.
		 * Those least significant bits which "represent" that alignment are therefore
		 * likely to be some static value (e.g. zero) and can be cut off for a better hash value.
		 */
		static const size_t shift = static_cast<size_t>(std::log2(1 + sizeof(net::socket)));

		return reinterpret_cast<size_t>(&val) >> shift;
	}
};

template<>
struct equal_to<net::socket> {
	bool operator()(const net::socket &left, const net::socket &right) const {
		return &left == &right;
	}
};

} // namespace std

#endif // nodecc_net_socket_h
