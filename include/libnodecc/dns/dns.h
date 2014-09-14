#ifndef nodecc_dns_dns_h
#define nodecc_dns_dns_h

#include <functional>
#include <memory>
#include <string>

#include "../uv/loop.h"


struct addrinfo;


namespace dns {

typedef std::function<void(const std::shared_ptr<addrinfo> &res)> on_lookup_t;

void lookup(const on_lookup_t& cb, uv::loop& loop, const std::string& domain, const addrinfo* hints = nullptr);
void lookup(const on_lookup_t& cb, uv::loop& loop, const std::string& domain, uint16_t port, const addrinfo* hints = nullptr);
void lookup(const on_lookup_t& cb, uv::loop& loop, const std::string& domain, const std::string& service, const addrinfo* hints = nullptr);

} // namespace dns

#endif // nodecc_dns_dns_h
