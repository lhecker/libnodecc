#ifndef nodecc_util_fnv_h
#define nodecc_util_fnv_h

#include <type_traits>

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
	static_assert(sizeof(std::size_t) == 4 || sizeof(std::size_t) == 8, "node::http::fnv1a relies on 32/64 Bit systems");

	static constexpr std::size_t _basis = std::conditional<sizeof(std::size_t) == 4, std::integral_constant<uint32_t, 2166136261UL>, std::integral_constant<uint64_t, 14695981039346656037ULL>>::type::value;
	static constexpr std::size_t _prime = std::conditional<sizeof(std::size_t) == 4, std::integral_constant<uint32_t,   16777619UL>, std::integral_constant<uint64_t,        1099511628211ULL>>::type::value;

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
