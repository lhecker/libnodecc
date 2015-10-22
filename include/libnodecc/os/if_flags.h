#ifndef nodecc_os_if_flags_h
#define nodecc_os_if_flags_h

namespace node {

enum class af : uint32_t {
	ipv4 = 0x00000001,
	ipv6 = 0x00000002,
	any  = 0x00000003,
};

/*
 * Existing implementations include libuv and chromium:
 * bsd:   https://chromium.googlesource.com/chromium/src/net/+/00b3f640998c13f26e3840cabf3c43d01b8d4f08/base/net_util_mac.cc
 * linux: https://chromium.googlesource.com/chromium/src/net/+/00b3f640998c13f26e3840cabf3c43d01b8d4f08/base/net_util_linux.cc
 * win:   https://chromium.googlesource.com/chromium/src/net/+/00b3f640998c13f26e3840cabf3c43d01b8d4f08/base/net_util_win.cc
 */
enum class iff : uint32_t {
	/**********************************
	 *         General flags          *
	 **********************************/

	none       = 0x00000000,
	any        = 0xffffffff,

	/**********************************
	 *        IPv4/IPv6 flags         *
	 **********************************/

	// posix: IFF_UP
	// win: IfOperStatusUp || IfOperStatusDown
	up         = 0x00000001,

	// posix: IFF_RUNNING
	// win: IfOperStatusUp
	running    = 0x00000002,

	// posix: IFF_LOOPBACK
	// win: IF_TYPE_SOFTWARE_LOOPBACK
	loopback   = 0x00000004,

	// posix: IFF_BROADCAST
	// win: always true
	broadcast  = 0x00000008,

	// posix: IFF_MULTICAST
	// win: !IP_ADAPTER_NO_MULTICAST
	multicast  = 0x00000010,

	/**********************************
	 *           IPv6 flags           *
	 **********************************/

	linklocal  = 0x00010000,

	// bsd: IN6_IFF_TENTATIVE
	// linux: RTM_GETADDR + IFA_F_TENTATIVE
	// win: IpDadStateTentative
	tentative  = 0x00020000,

	// bsd: IN6_IFF_DUPLICATED
	// linux: IFA_F_DADFAILED
	// win: IpDadStateDuplicate
	duplicated = 0x00040000,

	// bsd: IN6_IFF_DETACHED
	// linux: always false
	// win: always false
	detached   = 0x00080000,

	// bsd: IN6_IFF_DEPRECATED
	// linux: RTM_GETADDR + IFA_F_DEPRECATED
	// win: PreferredLifetime == 0
	deprecated = 0x00100000,

	// bsd: IN6_IFF_TEMPORARY
	// linux: RTM_GETADDR + IFA_F_TEMPORARY
	// win: PrefixOrigin == IpPrefixOriginRouterAdvertisement && SuffixOrigin == IpSuffixOriginRandom
	temporary  = 0x00200000,

	// bsd: IN6_IFF_NOTREADY
	// linux: RTM_GETADDR + IFA_F_OPTIMISTIC | IFA_F_DADFAILED | IFA_F_TENTATIVE
	// win: !IpDadStatePreferred
	notready   = 0x00400000,
};

constexpr iff operator|(iff a, iff b) {
	return static_cast<iff>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

} // namespace node

#endif // nodecc_os_if_flags_h
