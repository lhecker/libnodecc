#ifndef node_cpp_dns_dns_h
#define node_cpp_dns_dns_h

#include <functional>
#include <string>
#include <memory>


struct addrinfo;
typedef struct uv_loop_s uv_loop_t;


namespace dns {
	typedef std::function<void(int err, addrinfo *res)> on_lookup_t;

	void lookup(uv_loop_t *loop, const std::string &domain, const on_lookup_t &cb);
}

#endif
