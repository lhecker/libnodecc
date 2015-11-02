#include "libnodecc/stream.h"


namespace node {
namespace stream {
namespace detail {

decltype(readable_base::end_event) readable_base::end_event;
decltype(writable_base::drain_event) writable_base::drain_event;

} // namespace detail
} // namespace stream
} // namespace node
