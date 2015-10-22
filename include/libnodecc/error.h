#ifndef nodecc_error_h
#define nodecc_error_h

#include <system_error>


namespace node {
namespace uv {

std::error_code to_error(int ret) noexcept(true);
void check(int ret) noexcept(false);

} // namespace uv

namespace util {

void throw_errno() noexcept(false);

} // namespace util
} // namespace node

#endif // nodecc_error_h
