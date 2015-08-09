#ifndef nodecc_buffer_hashed_buffer_view_h
#define nodecc_buffer_hashed_buffer_view_h

#include "buffer_view.h"

#include "_hashed_trait.h"


namespace node {

class hashed_buffer_view : public buffer_view, public detail::hashed_trait<hashed_buffer_view> {
	friend class literal_string;

public:
	constexpr hashed_buffer_view() : buffer_view(), hashed_type(0) {}
	constexpr hashed_buffer_view(const hashed_buffer_view& other) : buffer_view(other.data(), other.size()), hashed_type(other.const_hash()) {}
	constexpr hashed_buffer_view(const void* data, std::size_t size) : buffer_view(data, size), hashed_type(0) {}

	constexpr hashed_buffer_view(const buffer_view& other) : buffer_view(other.data(), other.size()), hashed_type(0) {}

	hashed_buffer_view& operator=(const buffer_view& other);
	hashed_buffer_view& operator=(const hashed_buffer_view& other);

	using hashed_type::equals;

protected:
	// for literal_string
	constexpr hashed_buffer_view(const void* data, std::size_t size, std::size_t hash) : buffer_view(data, size), hashed_type(hash) {}
}; // class hashed_buffer_view

} // namespace node


template<>
struct std::hash<node::hashed_buffer_view> {
	std::size_t operator()(const node::hashed_buffer_view& buf) const {
		return buf.hash();
	}
};

template<>
struct std::equal_to<node::hashed_buffer_view> {
	bool operator()(const node::hashed_buffer_view& lhs, const node::hashed_buffer_view& rhs) const {
		return lhs.equals(rhs);
	}
};

#endif // nodecc_buffer_hashed_buffer_view_h
