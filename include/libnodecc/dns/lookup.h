#ifndef nodecc_dns_dns_h
#define nodecc_dns_dns_h

#include <functional>
#include <memory>

#include "../buffer.h"


struct addrinfo;

namespace node {
class loop;
}


namespace node {
namespace dns {

typedef std::function<void(int err, const std::shared_ptr<addrinfo> &res)> on_lookup_t;

void lookup(on_lookup_t cb, node::loop& loop, const node::string& domain, const addrinfo* hints = nullptr);
void lookup(on_lookup_t cb, node::loop& loop, const node::string& domain, uint16_t port, const addrinfo* hints = nullptr);
void lookup(on_lookup_t cb, node::loop& loop, const node::string& domain, const node::string& service, const addrinfo* hints = nullptr);

} // namespace dns
} // namespace node

#endif // nodecc_dns_dns_h
