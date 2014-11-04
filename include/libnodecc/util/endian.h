#ifndef nodecc_util_endian_h
#define nodecc_util_endian_h

#include "_endian.hpp"

#define NODE_LITTLE_ENDIAN BOOST_LITTLE_ENDIAN
#define NODE_BIG_ENDIAN    BOOST_BIG_ENDIAN

/*
 * Microsoft uses a version of htonl etc. which calls into ws2_32.dll.
 * Obviously this is very very slow ---> We fix this.
 */
#if _MSC_VER
# include <intrin.h>
# if NODE_BIG_ENDIAN
#  define ntohl(x) (x)
#  define htonl(x) (x)
# else
#  define ntohl(x) _byteswap_ulong(x)
#  define htonl(x) _byteswap_ulong(x)
# endif
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
# if NODE_BIG_ENDIAN
#  define ntohl(x) (x)
#  define htonl(x) (x)
# else
#  define ntohl(x) __builtin_bswap32(x)
#  define htonl(x) __builtin_bswap32(x)
# endif
#else
# include <arpa/inet.h>
#endif

#endif // nodecc_util_endian_h
