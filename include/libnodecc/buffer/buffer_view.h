#ifndef nodecc_buffer_buffer_view_h
#define nodecc_buffer_buffer_view_h

#include <functional>
#include <string>
#include <vector>

#include "../util/fnv.h"


namespace node {

class buffer_view;
class buffer;
class mutable_buffer;


inline namespace literals {
	constexpr buffer_view operator "" _buffer_view(const char*, std::size_t) noexcept;
} // inline namespace literals


/**
 * A immutable view on a memory area.
 *
 * This class can be used as a parameter for any function that deals with
 * memory areas, but only deal with them as long as the function call lasts.
 * This leads to some optimizations: This class can thus be initialized with
 * other buffers, strings and pointers, without worrying about whether
 */
class buffer_view {
	friend class buffer;
	friend class mutable_buffer;

	friend bool operator==(buffer_view&, buffer_view&) noexcept;
	friend bool operator!=(buffer_view&, buffer_view&) noexcept;

	friend constexpr node::buffer_view literals::operator "" _buffer_view(const char*, std::size_t) noexcept;

public:
	constexpr buffer_view() : _data(nullptr), _size(0), _hash(0) {}
	constexpr buffer_view(const buffer_view& other) : _data(other._data), _size(other._size), _hash(other._hash) {}
	constexpr buffer_view(const void* data, std::size_t size) : _data(const_cast<void*>(data)), _size(size), _hash(0) {}

	buffer_view& operator=(const buffer_view& other);

	template<typename T>
	explicit buffer_view(const std::vector<T>& vec) noexcept : buffer_view(vec.data(), vec.size()) {}

	template<typename CharT>
	explicit buffer_view(const CharT* str) noexcept : buffer_view(const_cast<CharT*>(str), std::char_traits<CharT>::length(str) * sizeof(CharT)) {}

	template<typename CharT, typename traits, typename Allocator>
	explicit buffer_view(const std::basic_string<CharT, traits, Allocator>& str) noexcept : buffer_view(str.data(), str.size() * sizeof(CharT)) {}


	template<typename T = uint8_t>
	constexpr T* data() const noexcept {
		return static_cast<T*>(this->_data);
	}

	constexpr operator void*() const noexcept {
		return this->data<void>();
	}

	constexpr operator char*() const noexcept {
		return this->data<char>();
	}

	constexpr operator unsigned char*() const noexcept {
		return this->data<unsigned char>();
	}

	constexpr operator const void*() const noexcept {
		return this->data<const void>();
	}

	constexpr operator const char*() const noexcept {
		return this->data<const char>();
	}

	constexpr operator const unsigned char*() const noexcept {
		return this->data<const unsigned char>();
	}

	constexpr uint8_t& operator[](std::size_t pos) const noexcept {
		return this->data<uint8_t>()[pos];
	}

	constexpr operator bool() const noexcept {
		return this->_data != nullptr;
	}

	constexpr bool empty() const noexcept {
		return this->_data == nullptr;
	}

	constexpr uint8_t* get() const noexcept {
		return this->data<uint8_t>();
	}

	constexpr std::size_t size() const noexcept {
		return this->_size;
	}

	constexpr std::size_t const_hash() const noexcept {
		return this->_hash;
	}


	std::size_t hash() noexcept;

	int compare(std::size_t pos1, std::size_t size1, const void* data2, std::size_t size2) const noexcept;

	inline int compare(std::size_t size1, const void* data2, std::size_t size2) const noexcept {
		return this->compare(0, size1, data2, size2);
	}

	inline int compare(const void* data2, std::size_t size2) const noexcept {
		return this->compare(0, this->size(), data2, size2);
	}

	inline int compare(const buffer_view& other) const noexcept {
		return this->compare(0, this->size(), other.get(), other.size());
	}

	template<typename CharT>
	inline int compare(const CharT* str) const noexcept {
		return this->compare(static_cast<const void*>(str), std::char_traits<CharT>::length(str));
	}


	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
	std::basic_string<CharT, Traits, Allocator> to_string() const {
		return std::basic_string<CharT, Traits, Allocator>(reinterpret_cast<CharT*>(this->_data), this->_size);
	}

private:
	constexpr buffer_view(const void* data, std::size_t size, std::size_t hash) : _data(const_cast<void*>(data)), _size(size), _hash(hash) {}

	void* _data;
	std::size_t _size;
	std::size_t _hash;
}; // class buffer_view


inline namespace literals {
	constexpr node::buffer_view operator "" _buffer_view(const char* str, std::size_t len) noexcept {
		return node::buffer_view(str, len, node::util::fnv1a(str, len));
	}
} // inline namespace literals

} // namespace node


template<>
struct std::hash<node::buffer_view> {
	std::size_t operator()(const node::buffer_view& buf) const {
		return const_cast<node::buffer_view&>(buf).hash();
	}
};

template<>
struct std::equal_to<node::buffer_view> {
	bool operator()(const node::buffer_view& lhs, const node::buffer_view& rhs) const {
		return const_cast<node::buffer_view&>(lhs).hash() == const_cast<node::buffer_view&>(rhs).hash() && lhs.compare(rhs) == 0;
	}
};

#endif // nodecc_buffer_buffer_view_h
