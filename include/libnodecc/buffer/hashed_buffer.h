#ifndef nodecc_buffer_hashed_buffer_h
#define nodecc_buffer_hashed_buffer_h

#include "buffer.h"


namespace node {

class hashed_buffer : public buffer {
public:
	hashed_buffer() : buffer(), _hash(0) {}
	hashed_buffer(const hashed_buffer& other) : buffer(other), _hash(other._hash) {}
	hashed_buffer(const void* data, std::size_t size) : buffer(data, size), _hash(0) {}

	hashed_buffer(const buffer& other) : buffer(other), _hash(0) {}

	hashed_buffer(const literal_string& other) noexcept : buffer(other, node::weak), _hash(other._hash) {}

	hashed_buffer& operator=(const buffer_view& other);
	hashed_buffer& operator=(const buffer& other);
	hashed_buffer& operator=(const hashed_buffer_view& other);


	std::size_t hash() const noexcept;

	bool equals(const hashed_buffer_view& other) const noexcept;

protected:
	std::size_t _hash;
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
