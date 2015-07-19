#ifndef nodecc_util_fnv_h
#define nodecc_util_fnv_h

#include <climits>
#include <cstdint>


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

class buffer_view;


namespace util {

namespace detail {

template<typename T>
struct fnv1a_statics;

template<>
struct fnv1a_statics<uint32_t> {
	constexpr static uint32_t _basis = 2166136261UL;
	constexpr static uint32_t _prime = 16777619UL;
};

template<>
struct fnv1a_statics<uint64_t> {
	constexpr static uint64_t _basis = 14695981039346656037ULL;
	constexpr static uint64_t _prime = 1099511628211ULL;
};

} // namespace detail


template<typename T>
struct fnv1a : public detail::fnv1a_statics<T> {
	static T hash(const node::buffer_view& view) {
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
