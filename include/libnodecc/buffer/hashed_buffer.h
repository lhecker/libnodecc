#ifndef nodecc_buffer_hashed_buffer_h
#define nodecc_buffer_hashed_buffer_h

#include "buffer.h"

#include "_hashed_trait.h"


namespace node {

class hashed_buffer : public buffer, public detail::hashed_trait<hashed_buffer> {
public:
	hashed_buffer() : buffer(), hashed_type(0) {}
	hashed_buffer(const hashed_buffer& other) : buffer(other), hashed_type(other.const_hash()) {}
	hashed_buffer(const void* data, std::size_t size) : buffer(data, size), hashed_type(0) {}

	hashed_buffer(const buffer& other) : buffer(other), hashed_type(0) {}

	hashed_buffer(const literal_string& other) noexcept : buffer(other, buffer_flags::weak), hashed_type(other.const_hash()) {}

	hashed_buffer& operator=(const buffer_view& other);
	hashed_buffer& operator=(const buffer& other);
	hashed_buffer& operator=(const hashed_buffer_view& other);

	using hashed_type::equals;
};

} // namespace node


template<>
struct std::hash<node::hashed_buffer> {
	std::size_t operator()(const node::hashed_buffer& buf) const {
		return buf.hash();
	}
};

template<>
struct std::equal_to<node::hashed_buffer> {
	bool operator()(const node::hashed_buffer& lhs, const node::hashed_buffer& rhs) const {
		return lhs.equals(rhs);
	}
};

#endif // nodecc_buffer_hashed_buffer_h
