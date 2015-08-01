#ifndef nodecc_buffer_hashed_buffer_view_h
#define nodecc_buffer_hashed_buffer_view_h

#include "buffer_view.h"


namespace node {

class hashed_buffer;
class literal_buffer_view;

inline namespace literals {
	constexpr literal_buffer_view operator "" _view(const char*, std::size_t) noexcept;
} // inline namespace literals


/**
 * A immutable view on a memory area.
 *
 * This class can be used as a parameter for any function that deals with
 * memory areas, but only deal with them as long as the function call lasts.
 * This leads to some optimizations: This class can thus be initialized with
 * other buffers, strings and pointers, without worrying about whether
 */
class hashed_buffer_view : public buffer_view {
	friend class hashed_buffer;
	friend class literal_buffer_view;

	friend constexpr literal_buffer_view literals::operator "" _view(const char*, std::size_t) noexcept;

public:
	constexpr hashed_buffer_view() : buffer_view(), _hash(0) {}
	constexpr hashed_buffer_view(const hashed_buffer_view& other) : buffer_view(other.data(), other.size()), _hash(other._hash) {}
	constexpr hashed_buffer_view(const void* data, std::size_t size) : buffer_view(data, size), _hash(0) {}

	constexpr hashed_buffer_view(const buffer_view& other) : buffer_view(other.data(), other.size()), _hash(0) {}

	hashed_buffer_view& operator=(const buffer_view& other);
	hashed_buffer_view& operator=(const hashed_buffer_view& other);

	std::size_t hash() const noexcept;

	bool equals(const hashed_buffer_view& other) const noexcept;

protected:
	constexpr hashed_buffer_view(const void* data, std::size_t size, std::size_t hash) : buffer_view(data, size), _hash(hash) {}

	std::size_t _hash;
}; // class hashed_buffer_view


class literal_buffer_view : public hashed_buffer_view {
public:
	constexpr literal_buffer_view(const void* data, std::size_t size, std::size_t hash) : hashed_buffer_view(data, size, hash) {}
}; // class literal_buffer_view

} // namespace node


#include "../util/fnv.h"


namespace node {

inline namespace literals {
	constexpr literal_buffer_view operator "" _view(const char* str, std::size_t len) noexcept {
		return literal_buffer_view(str, len, node::util::fnv1a<std::size_t>::const_hash(str, len));
	}
} // inline namespace literals

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
