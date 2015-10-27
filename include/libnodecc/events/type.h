#ifndef nodecc_events_type_h
#define nodecc_events_type_h

#include <map>
#include <type_traits>


namespace node {
namespace events {

template<typename T>
struct type {
	constexpr type() {}
};

} // namespace events
} // namespace node

#endif // nodecc_events_type_h
