#include "libnodecc/error.h"

#ifdef __GNUC__
# define unlikely(x) __builtin_expect((x), 0)
#else
# define unlikely(x) (x)
#endif


namespace node {
namespace uv {

std::error_code to_error(int ret) noexcept(true) {
	return std::error_code(-ret, std::generic_category());
}

void check(int ret) noexcept(false) {
	if (unlikely(ret < 0)) {
		throw std::system_error(to_error(ret));
	}
}

} // namespace uv

namespace util {

void throw_errno() noexcept(false) {
	throw std::system_error(errno, std::generic_category());
}

} // namespace util
} // namespace node
