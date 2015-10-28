#ifndef nodecc_events_type_h
#define nodecc_events_type_h

namespace node {
namespace events {

namespace detail {
struct base_type {};
} // namespace detail

template<typename T>
struct type : detail::base_type {
	constexpr type() {}
};

} // namespace events
} // namespace node

#endif // nodecc_events_type_h
