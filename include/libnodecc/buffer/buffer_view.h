#ifndef nodecc_buffer_buffer_view_h
#define nodecc_buffer_buffer_view_h

#include <functional>
#include <string>
#include <vector>


namespace node {

class buffer;
class mutable_buffer;


/**
 * A immutable view on a memory area.
 *
 * This class can be used as a parameter for any function that deals with
 * memory areas, but only deal with them as long as the function call lasts.
 * This leads to some optimizations: This class can thus be initialized with
 * other buffers, strings and pointers, without worrying about whether
 */
class buffer_view {
	friend class node::buffer;
	friend class node::mutable_buffer;

public:
	constexpr buffer_view() : _data(nullptr), _size(0) {}
	constexpr buffer_view(const buffer_view& other) : _data(other._data), _size(other._size) {}

	buffer_view& operator=(const buffer_view& other);

	buffer_view(const void* data, std::size_t size) noexcept;

	template<typename T>
	buffer_view(const std::vector<T>& vec) noexcept : buffer_view(vec.data(), vec.size()) {}

	template<typename charT>
	buffer_view(const charT* str) noexcept : buffer_view(const_cast<charT*>(str), std::char_traits<charT>::length(str) * sizeof(charT)) {}

	template<typename charT, typename traits, typename Allocator>
	buffer_view(const std::basic_string<charT, traits, Allocator>& str) noexcept : buffer_view(str.data(), str.size() * sizeof(charT)) {}


	template<typename T = uint8_t>
	inline T* data() const noexcept {
		return static_cast<T*>(this->_data);
	}

	inline operator void*() const noexcept {
		return this->data<void>();
	}

	inline operator char*() const noexcept {
		return this->data<char>();
	}

	inline operator unsigned char*() const noexcept {
		return this->data<unsigned char>();
	}

	inline operator const void*() const noexcept {
		return this->data<const void>();
	}

	inline operator const char*() const noexcept {
		return this->data<const char>();
	}

	inline operator const unsigned char*() const noexcept {
		return this->data<const unsigned char>();
	}

	inline uint8_t& operator[](std::size_t pos) noexcept {
		return this->data<uint8_t>()[pos];
	}

	inline const uint8_t& operator[](std::size_t pos) const noexcept {
		return this->data<uint8_t>()[pos];
	}

	inline operator bool() const noexcept {
		return this->_data != nullptr;
	}

	inline bool empty() const noexcept {
		return this->_data == nullptr;
	}

	inline uint8_t* get() const noexcept {
		return this->data<uint8_t>();
	}

	inline std::size_t size() const noexcept {
		return this->_size;
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
	inline std::basic_string<CharT, Traits, Allocator> to_string() const {
		return std::basic_string<CharT, Traits, Allocator>(reinterpret_cast<CharT*>(this->_data), this->_size);
	}

	friend bool operator==(const buffer_view& lhs, const buffer_view& rhs) noexcept;
	friend bool operator!=(const buffer_view& lhs, const buffer_view& rhs) noexcept;

private:
	void* _data;
	std::size_t _size;
};

} // namespace node


template<>
struct std::hash<node::buffer_view> {
	std::size_t operator()(const node::buffer_view& buf) const {
		std::size_t x = std::size_t(buf.data());
		return x + (x >> 3);
	}
};

template<>
struct std::equal_to<node::buffer_view> {
	bool operator()(const node::buffer_view& lhs, const node::buffer_view& rhs) const {
		return lhs.data() == rhs.data() && lhs.size() == rhs.size();
	}
};

#endif // nodecc_buffer_buffer_view_h
