#ifndef nodecc_buffer_hashed_view_h
#define nodecc_buffer_hashed_view_h

#include "buffer_view.h"


namespace node {

class hashed_view;

inline namespace literals {
	constexpr hashed_view operator "" _hashed_view(const char*, std::size_t) noexcept;
} // inline namespace literals


/**
 * A immutable view on a memory area.
 *
 * This class can be used as a parameter for any function that deals with
 * memory areas, but only deal with them as long as the function call lasts.
 * This leads to some optimizations: This class can thus be initialized with
 * other buffers, strings and pointers, without worrying about whether
 */
class hashed_view : public buffer_view {
	friend bool operator==(hashed_view&, hashed_view&) noexcept;
	friend bool operator!=(hashed_view&, hashed_view&) noexcept;

	friend constexpr node::hashed_view literals::operator "" _hashed_view(const char*, std::size_t) noexcept;

public:
	constexpr hashed_view() : buffer_view(), _hash(0) {}
	constexpr hashed_view(const hashed_view& other) : buffer_view(other.data(), other.size()), _hash(other._hash) {}
	constexpr hashed_view(const void* data, std::size_t size) : buffer_view(data, size), _hash(0) {}

	constexpr hashed_view(const buffer_view& other) : buffer_view(other.data(), other.size()), _hash(0) {}

	hashed_view& operator=(const buffer_view& other);
	hashed_view& operator=(const hashed_view& other);

	std::size_t hash() const noexcept;

	bool equals(const hashed_view& other) const noexcept;

protected:
	constexpr hashed_view(const void* data, std::size_t size, std::size_t hash) : buffer_view(data, size), _hash(hash) {}

	std::size_t _hash;
}; // class hashed_view

} // namespace node


#include "../util/fnv.h"


namespace node {

inline namespace literals {
	constexpr node::hashed_view operator "" _hashed_view(const char* str, std::size_t len) noexcept {
		return node::hashed_view(str, len, node::util::fnv1a<std::size_t>::const_hash(str, len));
	}
} // inline namespace literals

} // namespace node


template<>
struct std::hash<node::hashed_view> {
	std::size_t operator()(const node::hashed_view& buf) const {
		return buf.hash();
	}
};

template<>
struct std::equal_to<node::hashed_view> {
	bool operator()(const node::hashed_view& lhs, const node::hashed_view& rhs) const {
		return lhs.equals(rhs);
	}
};

#endif // nodecc_buffer_hashed_view_h
