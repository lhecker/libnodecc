#ifndef nodecc_buffer_literal_string_h
#define nodecc_buffer_literal_string_h

#include "hashed_buffer_view.h"


namespace node {

class literal_string : public hashed_buffer_view {
public:
	constexpr literal_string(const char* str, std::size_t len, std::size_t hash) : hashed_buffer_view(str, len, hash) {}
};


inline namespace literals {

constexpr literal_string operator "" _view(const char* str, std::size_t len) noexcept {
	return literal_string(str, len, node::util::fnv1a<std::size_t>::const_hash(str, len));
}

} // inline namespace literals
} // namespace node


template<>
struct std::hash<node::literal_string> {
	std::size_t operator()(const node::literal_string& buf) const {
		return buf.hash();
	}
};

template<>
struct std::equal_to<node::literal_string> {
	bool operator()(const node::literal_string& lhs, const node::literal_string& rhs) const {
		return lhs.equals(rhs);
	}
};

#endif // nodecc_buffer_literal_string_h
