#ifndef nodecc_events_symbol_h
#define nodecc_events_symbol_h

namespace node {
namespace events {

namespace detail {
struct base_symbol {};
} // namespace detail

template<typename T>
struct symbol : detail::base_symbol {
	constexpr symbol() {}
};

} // namespace events
} // namespace node

#endif // nodecc_events_symbol_h
