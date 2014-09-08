#ifndef nodecc_dns_dns_h
#define nodecc_dns_dns_h

#include <functional>
#include <memory>
#include <string>

#include "../uv/loop.h"


struct addrinfo;


namespace dns {

typedef std::function<void(const std::shared_ptr<addrinfo> &res)> on_lookup_t;

void lookup(uv::loop& loop, const std::string& domain, const on_lookup_t& cb);

} // namespace dns

#endif // nodecc_dns_dns_h
