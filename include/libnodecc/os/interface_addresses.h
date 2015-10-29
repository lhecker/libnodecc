#ifndef nodecc_os_interface_addresses_h
#define nodecc_os_interface_addresses_h

#include <array>
#include <uv.h>

#include "../buffer.h"
#include "../util/raw_vector.h"
#include "if_flags.h"


namespace node {
namespace os {

struct interface_address {
	interface_address(const char* name, bool is_ipv6) : name(name), is_ipv6(is_ipv6) {}

	const node::string name;
	const bool is_ipv6;

	union {
		struct sockaddr_in address4;
		struct sockaddr_in6 address6;
	} address;

	union {
		struct sockaddr_in netmask4;
		struct sockaddr_in6 netmask6;
	} netmask;

	node::string address_string() const;
};

std::vector<interface_address> interface_addresses(node::af type = af::any, node::iff whitelist = iff::up | iff::running, node::iff blacklist = iff::loopback | iff::linklocal | iff::detached | iff::deprecated | iff::temporary | iff::notready);

} // namespace os
} // namespace node

#endif // nodecc_os_interface_addresses_h
