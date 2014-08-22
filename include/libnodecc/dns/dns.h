#ifndef nodecc_dns_dns_h
#define nodecc_dns_dns_h

#include <functional>
#include <string>
#include <memory>


struct addrinfo;
typedef struct uv_loop_s uv_loop_t;


namespace dns {

typedef std::function<void(const std::shared_ptr<addrinfo> &res)> on_lookup_t;

void lookup(uv_loop_t *loop, const std::string &domain, const on_lookup_t &cb);

} // namespace dns

#endif // nodecc_dns_dns_h
