#ifndef nodecc_udp_socket_h
#define nodecc_udp_socket_h

#include "../uv/stream.h"


namespace node {
namespace udp {

enum class membership {
	join  = UV_JOIN_GROUP,
	leave = UV_LEAVE_GROUP,
};

enum class flags : unsigned int {
	none      = 0,
	ipv6only  = UV_UDP_IPV6ONLY,
	partial   = UV_UDP_PARTIAL,
	reuseaddr = UV_UDP_REUSEADDR,
};

constexpr flags operator|(flags a, flags b) {
	return static_cast<flags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

class socket : public node::uv::handle<uv_udp_t> {
public:
	explicit socket(node::loop& loop);

	void listen(const sockaddr& addr, node::udp::flags flags = flags::none);
	void listen4(uint16_t port = 0, const node::string& ip = node::literal_string("0.0.0.0", 7), node::udp::flags flags = flags::none);
	void listen6(uint16_t port = 0, const node::string& ip = node::literal_string("::0", 2), node::udp::flags flags = flags::none);

	void address(sockaddr& addr, int& len);
	uint16_t port();

	void set_membership(node::udp::membership membership, const node::string& multicast_addr, const node::string& interface_addr = node::string());
	void set_multicast_loop(bool on);
	void set_multicast_ttl(int ttl);
	void set_multicast_interface(const node::string& interface_addr);
	void set_broadcast(bool on);
	void set_ttl(int ttl);

	void resume();
	void pause();

	inline void write(sockaddr& addr, const node::buffer& chunk) {
		this->write(addr, &chunk, 1);
	}

	void write(sockaddr& addr, const node::buffer chunks[], size_t chunkcnt);

	node::callback<node::buffer()> alloc_callback;
	node::callback<void(const sockaddr& remote, const node::buffer chunks[], size_t chunkcnt)> data_callback;
	node::signal<void(int err)> error_signal;

protected:
	~socket() override = default;

	void _destroy() override;

	bool is_consuming() const noexcept {
		return this->_is_consuming;
	}

private:
	node::buffer _alloc_buffer;
	bool _is_consuming = false;
};

} // namespace udp
} // namespace node

#endif // nodecc_udp_socket_h
