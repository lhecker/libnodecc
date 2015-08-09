#ifndef nodecc_buffer_hashed_string_h
#define nodecc_buffer_hashed_string_h

#include "string.h"

#include "_hashed_trait.h"


namespace node {

class hashed_string : public string, public detail::hashed_trait<hashed_string> {
	friend class literal_string;

public:
	hashed_string() : string(), hashed_type(0) {}
	hashed_string(const hashed_string& other) : string(other), hashed_type(other.const_hash()) {}

	hashed_string(const string& other) : string(other), hashed_type(0) {}

	hashed_string& operator=(const string& other);
	hashed_string& operator=(const hashed_string& other);

	using hashed_type::equals;
};

} // namespace node


template<>
struct std::hash<node::hashed_string> {
	std::size_t operator()(const node::hashed_string& buf) const {
		return buf.hash();
	}
};

template<>
struct std::equal_to<node::hashed_string> {
	bool operator()(const node::hashed_string& lhs, const node::hashed_string& rhs) const {
		return lhs.equals(rhs);
	}
};

#endif // nodecc_buffer_hashed_string_h
