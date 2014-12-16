#include "libnodecc/util/math.h"

#include <cmath>


#if defined(NODE_HAS_BUILTIN_CLZ)

#include <limits>

unsigned int node::util::digits2(unsigned int n) {
	return n ? std::numeric_limits<unsigned int>::digits - __builtin_clz(n) : 0;
}

unsigned int node::util::digits2(unsigned long n) {
	return n ? std::numeric_limits<unsigned long>::digits - __builtin_clzl(n) : 0;
}

unsigned int node::util::digits2(unsigned long long n) {
	return n ? std::numeric_limits<unsigned long long>::digits - __builtin_clzll(n) : 0;
}

#elif defined(NODE_HAS_BUILTIN_BSR)

#include <intrin.h>
#include <limits>

unsigned int node::util::digits2(unsigned int n) {
	// it's guaranteed that long is at least as large as an int
	return node::util::digits2(static_cast<unsigned long>(n));
}

unsigned int node::util::digits2(unsigned long n) {
	unsigned long idx;
	if (_BitScanReverse(&idx, n)) {
		return std::numeric_limits<unsigned long>::digits - idx;
	} else {
		return 0;
	}
}

unsigned int node::util::digits2(unsigned long long n) {
	static_assert(std::numeric_limits<unsigned long long>::digits == 64, "unsigned long long needs to be 64Bit for _BitScanReverse64 to work");

	unsigned long idx;
	if (_BitScanReverse64(&idx, n)) {
		return std::numeric_limits<unsigned long long>::digits - idx;
	} else {
		return 0;
	}
}

#else

#include <cstdint>

unsigned int node::util::digits2(unsigned int n) {
	// it's guaranteed that long is at least as large as an int
	return node::util::digits2(static_cast<unsigned long>(n));
}

unsigned int node::util::digits2(unsigned long n) {
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
		return 32;
	} else {
		n |= n + 1; // output is not log2() but the amount of digits ---> ceil()
		return index32[(uint32_t)(n * UINT32_C(0x07c4acdd)) >> 27];
	}
#else
	return node::util::digits2(static_cast<unsigned long long>(n));
#endif
}

unsigned int node::util::digits2(unsigned long long n) {
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
		return 64;
	} else {
		return index64[(n * UINT64_C(0x03f79d71b4cb0a89)) >> 58];
	}
}

#endif

unsigned int node::util::digits(size_t n, uint8_t base) {
	switch (base) {
	case 0:
	case 1:
		return 0;
	case 2:
		return node::util::digits2(n);
#if SIZE_T_MAX <= UINT32_MAX
	case 10:
		return (n >= 1000000000ul) ? 10
		     : (n >= 100000000ul) ? 9
		     : (n >= 10000000ul) ? 8
		     : (n >= 1000000ul) ? 7
		     : (n >= 100000ul) ? 6
		     : (n >= 10000ul) ? 5
		     : (n >= 1000ul) ? 4
		     : (n >= 100ul) ? 3
		     : (n >= 10ul) ? 2
			 : 1;
#elif SIZE_T_MAX == UINT64_MAX
	case 10:
		return (n >= 10000000000000000000ull) ? 20
		     : (n >= 1000000000000000000ull) ? 19
		     : (n >= 100000000000000000ull) ? 18
		     : (n >= 10000000000000000ull) ? 17
		     : (n >= 1000000000000000ull) ? 16
		     : (n >= 100000000000000ull) ? 15
		     : (n >= 10000000000000ull) ? 14
		     : (n >= 1000000000000ull) ? 13
		     : (n >= 100000000000ull) ? 12
		     : (n >= 10000000000ull) ? 11
		     : (n >= 1000000000ull) ? 10
		     : (n >= 100000000ull) ? 9
		     : (n >= 10000000ull) ? 8
		     : (n >= 1000000ull) ? 7
		     : (n >= 100000ull) ? 6
		     : (n >= 10000ull) ? 5
		     : (n >= 1000ull) ? 4
		     : (n >= 100ull) ? 3
		     : (n >= 10ull) ? 2
			 : 1;
#endif
	default:
		// it's a power of 2
		if ((base & (base - 1)) == 0) {
			unsigned int m = node::util::digits2((unsigned int)base);
			return (node::util::digits2(n) + m - 2) / (m - 1);
		}

		return std::log(n) / std::log(base);
	}
}
