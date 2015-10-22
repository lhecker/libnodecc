#include "libnodecc/util/math.h"

#include <cmath>

#if defined(NODE_HAS_BUILTIN_CLZ)
# include <limits>
#elif defined(NODE_HAS_BUILTIN_BSR)
# include <intrin.h>
# include <limits>
#else
# include <cstdint>
#endif


namespace node {
namespace util {

#if defined(NODE_HAS_BUILTIN_CLZ)

unsigned int digits2(unsigned int n) {
	return n ? std::numeric_limits<unsigned int>::digits - __builtin_clz(n) : 0;
}

unsigned int digits2(unsigned long n) {
	return n ? std::numeric_limits<unsigned long>::digits - __builtin_clzl(n) : 0;
}

unsigned int digits2(unsigned long long n) {
	return n ? std::numeric_limits<unsigned long long>::digits - __builtin_clzll(n) : 0;
}

#elif defined(NODE_HAS_BUILTIN_BSR)

unsigned int digits2(unsigned int n) {
	// it's guaranteed that long is at least as large as an int
	return digits2(static_cast<unsigned long>(n));
}

unsigned int digits2(unsigned long n) {
	static_assert(std::numeric_limits<decltype(n)>::digits == 32, "_BitScanReverse needs uint32_t");

	unsigned long idx;

	if (_BitScanReverse(&idx, n)) {
		return idx;
	} else {
		return 0;
	}
}

#if !defined(NODE_WITHOUT_BUILTIN_BSR64)
unsigned int digits2(unsigned long long n) {
	static_assert(std::numeric_limits<decltype(n)>::digits == 64, "_BitScanReverse needs uint64_t");

	unsigned long idx;

	if (_BitScanReverse64(&idx, n)) {
		return idx;
	} else {
		return 0;
	}
}
#endif

#else // no builtin CLZ/BSR

unsigned int digits2(unsigned int n) {
	// it's guaranteed that long is at least as large as an int
	return digits2(static_cast<unsigned long>(n));
}

unsigned int digits2(unsigned long n) {
#if ULONG_MAX == UINT32_MAX
	// uses De Bruijn Multiplication
	static const unsigned int index32[32] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 32 };

	// set all bits below the most significant bit
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;

	if (n == UINT32_MAX) {
		return 32ul;
	} else {
		n |= n + 1; // output is not log2() but the amount of digits ---> ceil()
		return index32[(uint32_t)(n * UINT32_C(0x07c4acdd)) >> 27];
	}
#else
	return digits2(static_cast<unsigned long long>(n));
#endif
}

unsigned int digits2(unsigned long long n) {
	static_assert(std::numeric_limits<unsigned long long>::digits == 64, "unsigned long long needs to be 64Bit for this De Bruijn Multiplication to work");

	// see 32bit variant
	static const unsigned int index64[64] = { 0, 47, 1, 56, 48, 27, 2, 60, 57, 49, 41, 37, 28, 16, 3, 61, 54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4, 62, 46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45, 25, 39, 14, 33, 19, 30, 9, 24, 13, 18, 8, 12, 7, 6, 5, 63 };

	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	n |= n + 1;

	if (n == UINT64_MAX) {
		return 64ul;
	} else {
		return index64[(n * UINT64_C(0x03f79d71b4cb0a89)) >> 58];
	}
}

#endif

unsigned int digits(size_t n, uint8_t base) {
	switch (base) {
	case 0:
	case 1:
		return 0ul;
	case 2:
		return digits2(n);
#if SIZE_T_MAX <= UINT32_MAX
	case 10:
		return (n >= 1000000000ul) ? 10ul
		     : (n >= 100000000ul) ? 9ul
		     : (n >= 10000000ul) ? 8ul
		     : (n >= 1000000ul) ? 7ul
		     : (n >= 100000ul) ? 6ul
		     : (n >= 10000ul) ? 5ul
		     : (n >= 1000ul) ? 4ul
		     : (n >= 100ul) ? 3ul
		     : (n >= 10ul) ? 2ul
			 : 1ul;
#elif SIZE_T_MAX == UINT64_MAX
	case 10:
		return (n >= 10000000000000000000ull) ? 20ul
		     : (n >= 1000000000000000000ull) ? 19ul
		     : (n >= 100000000000000000ull) ? 18ul
		     : (n >= 10000000000000000ull) ? 17ul
		     : (n >= 1000000000000000ull) ? 16ul
		     : (n >= 100000000000000ull) ? 15ul
		     : (n >= 10000000000000ull) ? 14ul
		     : (n >= 1000000000000ull) ? 13ul
		     : (n >= 100000000000ull) ? 12ul
		     : (n >= 10000000000ull) ? 11ul
		     : (n >= 1000000000ull) ? 10ul
		     : (n >= 100000000ull) ? 9ul
		     : (n >= 10000000ull) ? 8ul
		     : (n >= 1000000ull) ? 7ul
		     : (n >= 100000ull) ? 6ul
		     : (n >= 10000ull) ? 5ul
		     : (n >= 1000ull) ? 4ul
		     : (n >= 100ull) ? 3ul
		     : (n >= 10ull) ? 2ul
			 : 1ul;
#endif
	default:
		// it's a power of 2
		if ((base & (base - 1)) == 0) {
			const unsigned int a = digits2(static_cast<size_t>(n));
			const unsigned int b = digits2(static_cast<size_t>(base));
			return (a + b - 2ul) / (b - 1ul);
		}

		return (unsigned int)(std::log(n) / std::log(base));
	}
}

} // namespace util
} // namespace node
