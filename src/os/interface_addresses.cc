#include "libnodecc/os/interface_addresses.h"

#include <ifaddrs.h>
#include <net/if.h>
#include <uv.h>

#include "libnodecc/error.h"


#if defined(__has_include) && __has_include(<netinet/in_var.h>)
# define NODE_IFF_BSD
# include <sys/ioctl.h>
# include <netinet/in_var.h>
#endif


namespace node {
namespace os {

node::string interface_address::address_string() const {
	char buf[std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
	const void* addr = this->is_ipv6 ? (void*)&this->address.address6.sin6_addr : (void*)&this->address.address4.sin_addr;

	if (0 == uv_inet_ntop(this->address.address4.sin_family, addr, buf, sizeof(buf))) {
		return node::string(buf);
	}

	throw std::runtime_error("invalid address");
}


static constexpr bool iff_filter(node::iff wl, node::iff bl, node::iff flag, bool cond) {
	return (!(static_cast<uint32_t>(wl) & static_cast<uint32_t>(flag)) || cond) && (!(static_cast<uint32_t>(bl) & static_cast<uint32_t>(flag)) || !cond);
}


#ifdef NODE_IFF_BSD
static bool check_in6_iff_bsd(ifaddrs* ifa, int& sockfd, node::iff wl, node::iff bl) {
	if (ifa->ifa_addr->sa_family != AF_INET6) {
		return true;
	}

	if (sockfd == -1) {
		if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
			node::util::throw_errno();
		}
	}

	in6_ifreq ifr6;
	memset(&ifr6, 0, sizeof(ifr6));
	strlcpy(ifr6.ifr_name, ifa->ifa_name, sizeof(ifr6.ifr_name));
	memcpy(&ifr6.ifr_ifru.ifru_addr, ifa->ifa_addr, ((sockaddr_in6*)ifa->ifa_addr)->sin6_len);

	if (ioctl(sockfd, SIOCGIFAFLAG_IN6, &ifr6) < 0) {
		node::util::throw_errno();
	}

	return true
		&& iff_filter(wl, bl, iff::linklocal,  ((sockaddr_in6*)ifa->ifa_addr)->sin6_scope_id != 0)
		&& iff_filter(wl, bl, iff::tentative,  ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_TENTATIVE)
		&& iff_filter(wl, bl, iff::duplicated, ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DUPLICATED)
		&& iff_filter(wl, bl, iff::detached,   ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DETACHED)
		&& iff_filter(wl, bl, iff::deprecated, ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DEPRECATED)
		&& iff_filter(wl, bl, iff::temporary,  ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_TEMPORARY)
		&& iff_filter(wl, bl, iff::notready,   ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_NOTREADY);
}
#endif


std::vector<interface_address> interface_addresses(node::af type, node::iff wl, node::iff bl) {
	std::vector<interface_address> addresses;
	ifaddrs* ifas = nullptr;

	if (getifaddrs(&ifas) < 0) {
		node::util::throw_errno();
	}

	auto ifa = ifas;

#ifdef NODE_IFF_BSD
	const bool check_in6_iff = static_cast<uint32_t>(wl | bl) & 0xffff0000;
	int sockfd = -1;
#endif

	try {
		while (ifa != nullptr) {
			const bool if4 = (static_cast<uint32_t>(type) & static_cast<uint32_t>(af::ipv4)) && ifa->ifa_addr->sa_family == AF_INET;
			const bool if6 = (static_cast<uint32_t>(type) & static_cast<uint32_t>(af::ipv6)) && ifa->ifa_addr->sa_family == AF_INET6;

			if (ifa->ifa_addr != nullptr
				&& (if4 || if6)
				&& iff_filter(wl, bl, iff::up,        ifa->ifa_flags & IFF_UP)
				&& iff_filter(wl, bl, iff::running,   ifa->ifa_flags & IFF_RUNNING)
				&& iff_filter(wl, bl, iff::loopback,  ifa->ifa_flags & IFF_LOOPBACK)
				&& iff_filter(wl, bl, iff::broadcast, ifa->ifa_flags & IFF_BROADCAST)
				&& iff_filter(wl, bl, iff::multicast, ifa->ifa_flags & IFF_MULTICAST)
#ifdef NODE_IFF_BSD
				&& (!check_in6_iff || check_in6_iff_bsd(ifa, sockfd, wl, bl))
#endif
			) {
				addresses.emplace_back(ifa->ifa_name, ifa->ifa_addr->sa_family == AF_INET6);

				auto& address = addresses.back();

				if (ifa->ifa_addr->sa_family == AF_INET) {
					address.address.address4 = *((sockaddr_in*)ifa->ifa_addr);
				} else {
					address.address.address6 = *((sockaddr_in6*)ifa->ifa_addr);
				}
				if (ifa->ifa_netmask->sa_family == AF_INET) {
					address.netmask.netmask4 = *((sockaddr_in*)ifa->ifa_netmask);
				} else {
					address.netmask.netmask6 = *((sockaddr_in6*)ifa->ifa_netmask);
				}
			}

			ifa = ifa->ifa_next;
		}
	} catch (...) {
		freeifaddrs(ifas);
		throw;
	}

	return addresses;
}

} // namespace os
} // namespace node
