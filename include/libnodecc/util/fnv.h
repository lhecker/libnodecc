#ifndef nodecc_util_fnv_h
#define nodecc_util_fnv_h

#include <climits>
#include <cstdint>

#include "../buffer.h"







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

#ifdef SIZE_T_MAX
# define FNV1_SIZE_MAX SIZE_T_MAX
#elif SIZE_MAX
# define FNV1_SIZE_MAX SIZE_MAX
#endif

#if !defined(FNV1_SIZE_MAX) || !(FNV1_SIZE_MAX == UINT32_MAX || FNV1_SIZE_MAX == UINT64_MAX)
# error "node::util::fnv1a is not supported"
#endif

#define fnv1a_generator(_type_, _base_, _prime_)                                                               \
	template<typename CharT>                                                                                   \
	static constexpr std::size_t fnv1a(const CharT* str, const _type_ val = _base_) {                          \
		return (*str == '\0') ? val : fnv1a(str + 1, (val ^ _type_(*str)) * _prime_);                          \
	}                                                                                                          \
	                                                                                                           \
	template<typename CharT>                                                                                   \
	static constexpr std::size_t fnv1a(const CharT* str, const std::size_t len, const _type_ val = _base_) {   \
		return (len == 0) ? val : fnv1a(str + 1, len - 1, (val ^ _type_(*str)) * _prime_);                     \
	}                                                                                                          \

#if FNV1_SIZE_MAX == UINT32_MAX
  fnv1a_generator(uint32_t, 2166136261UL, 16777619UL)
#else
  fnv1a_generator(uint64_t, 14695981039346656037ULL, 1099511628211ULL)
#endif

#undef fnv1a_generator
#undef FNV1_SIZE_MAX

} // namespace util
} // namespace node

#endif // nodecc_util_fnv_h
