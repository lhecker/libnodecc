#ifndef nodecc_buffer_buffer_view_h
#define nodecc_buffer_buffer_view_h

#include <functional>
#include <string>
#include <vector>


namespace node {

class buffer_view;

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
	friend bool operator==(buffer_view&, buffer_view&) noexcept;
	friend bool operator!=(buffer_view&, buffer_view&) noexcept;

public:
	static constexpr std::size_t npos = -1;


	constexpr buffer_view() : _data(nullptr), _size(0) {}
	constexpr buffer_view(const buffer_view& other) : _data(other._data), _size(other._size) {}
	constexpr buffer_view(const void* data, std::size_t size) : _data(const_cast<void*>(data)), _size(size) {}


	template<typename CharT>
	explicit buffer_view(const CharT* str) noexcept : buffer_view(const_cast<CharT*>(str), std::char_traits<CharT>::length(str) * sizeof(CharT)) {}

	template<typename CharT, typename traits, typename Allocator>
	explicit buffer_view(const std::basic_string<CharT, traits, Allocator>& str) noexcept : buffer_view(str.data(), str.size() * sizeof(CharT)) {}


	buffer_view& operator=(const buffer_view& other);


	template<typename T = uint8_t>
	T* begin() const noexcept {
		return static_cast<T*>(this->_data);
	}

	template<typename T = uint8_t>
	const T* cbegin() const noexcept {
		return this->begin<T>();
	}

	template<typename T = uint8_t>
	T* end() const noexcept {
		const auto data = this->data<uint8_t>();
		return data ? static_cast<T*>(data + this->size()) : nullptr;
	}

	template<typename T = uint8_t>
	const T* cend() const noexcept {
		return this->end<T>();
	}


	template<typename T = uint8_t>
	T* data() const noexcept {
		return static_cast<T*>(this->_data);
	}

	uint8_t& operator[](std::size_t pos) const noexcept {
		return this->data()[pos];
	}

	constexpr std::size_t size() const noexcept {
		return this->_size;
	}

	constexpr operator bool() const noexcept {
		return this->_data != nullptr;
	}

	constexpr bool empty() const noexcept {
		return this->_data == nullptr;
	}


	bool equals(const buffer_view& other) const noexcept;

	int compare(const buffer_view& other, std::size_t pos1 = 0, std::size_t size1 = npos) const noexcept;

	std::size_t index_of(const char ch) const noexcept;
	std::size_t index_of(const buffer_view& other) const noexcept;


	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
	std::basic_string<CharT, Traits, Allocator> to_string() const {
		return std::basic_string<CharT, Traits, Allocator>(reinterpret_cast<CharT*>(this->_data), this->_size);
	}

	std::unique_ptr<char> c_str() const noexcept {
		char* str = nullptr;

		if (this->size()) {
			str = new char[this->size() + 1];
			memcpy(str, this->data(), this->size());
			str[this->size()] = '\0';
		}

		return std::unique_ptr<char>(str);
	}

protected:
	void* _data;
	std::size_t _size;
}; // class buffer_view

} // namespace node


#include "../util/fnv.h"


namespace node {

inline namespace literals {

constexpr node::buffer_view operator "" _buffer_view(const char* str, std::size_t len) noexcept {
	return node::buffer_view(str, len);
}

} // inline namespace literals

} // namespace node

#endif // nodecc_buffer_buffer_view_h
