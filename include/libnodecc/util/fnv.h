#ifndef nodecc_util_fnv_h
#define nodecc_util_fnv_h

/*
 * TODO: add tests!
 *   raw string
 *   uint32_t result
 *   uint64_t result
 *
 * "foobar"
 * 0xbf9cf968UL
 * 0x85944171f73967e8ULL
 *
 * "http://slashdot.org/"
 * 0xc3e20f5cUL
 * 0xa1031f8e7599d79cULL
 *
 * "\x40\x51\x4e\x54"
 * 0x4217a988UL
 * 0xe3b37596127cf208ULL
 *
 * "http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash"
 * 0xdd16ef45UL
 * 0xd9b957fb7fe794c5ULL
 */

namespace node {

namespace util {

template<typename T>
struct fnv1a {
	static constexpr std::size_t _basis = sizeof(std::size_t) == 4 ? 2166136261UL : sizeof(std::size_t) == 8 ? 14695981039346656037ULL : throw "unsupported";
	static constexpr std::size_t _prime = sizeof(std::size_t) == 4 ? 16777619UL   : sizeof(std::size_t) == 8 ? 1099511628211ULL        : throw "unsupported";

	static T hash(const buffer_view& view) {
		T hash = _basis;

		uint8_t* data = view.data();
		const uint8_t* data_end = data + view.size();

		for (; data < data_end; data++) {
			hash = (hash ^ static_cast<T>(*data)) * _prime;
		}

		return hash;
	}

	static constexpr T const_hash(const char* str, const std::size_t len, const T hash = _basis) {
		return (len == 0) ? hash : const_hash(str + 1, len - 1, (hash ^ static_cast<T>(*str)) * _prime);
	}
};

} // namespace util
} // namespace node

#endif // nodecc_util_fnv_h
