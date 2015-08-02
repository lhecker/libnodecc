#ifndef nodecc_buffer_hashed_string_h
#define nodecc_buffer_hashed_string_h

#include "string.h"


namespace node {

class hashed_string : public string {
	friend class literal_string;

public:
	hashed_string() : string(), _hash(0) {}
	hashed_string(const hashed_string& other) : string(other), _hash(other._hash) {}

	hashed_string(const string& other) : string(other), _hash(0) {}

	hashed_string& operator=(const string& other);
	hashed_string& operator=(const hashed_string& other);


	std::size_t hash() const noexcept;

	bool equals(const hashed_string& other) const noexcept;

protected:
	std::size_t _hash;
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
