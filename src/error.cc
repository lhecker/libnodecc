#include "libnodecc/error.h"


namespace node {
namespace uv {

std::error_code to_error(int ret) noexcept(true) {
	return std::error_code(-ret, std::generic_category());
}

void check(int ret) noexcept(false) {
	if (ret < 0) {
		throw std::system_error(to_error(ret));
	}
}

} // namespace uv
} // namespace node
