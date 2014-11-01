#ifndef nodecc_util_math_h
#define nodecc_util_math_h

#include <cstddef>
#include <cstdint>


#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
# define NODE_HAS_BUILTIN_CLZ // __builtin_clz() defined
#elif _MSC_VER >= 1400
# define NODE_HAS_BUILTIN_BSR // _BitScanReverse() defined
#endif


namespace node {
namespace util {

unsigned int digits2(unsigned int n);
unsigned int digits2(unsigned long n);
unsigned int digits2(unsigned long long n);

unsigned int digits(size_t n, uint8_t base);

template<typename T1, typename T2>
T1 ipow(T1 base, T2 exp) {
	T1 result = 1;

	while (exp) {
		if (exp & 1) result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}

} // namespace util
} // namespace node


#endif // nodecc_util_math_h
