#ifndef nodecc_util_string_h
#define nodecc_util_string_h

#include "buffer.h"


namespace util {

class string {
public:
	explicit string() : _buffer(), _used(0) {}
	explicit string(size_t size) : _buffer(size), _used(0) {}

	template<typename charT>
	void push_back(charT ch) {
		this->append(sizeof(charT));
		*reinterpret_cast<charT*>(this->_buffer.get() + this->_used) = ch;
		this->_used += sizeof(charT);
	}

	template<typename charT>
	string& append(const charT* data) {
		this->append(static_cast<const void*>(data), strlen(data));
		return *this;
	}

	template<typename charT>
	string& append(const charT* data, size_t count) {
		this->append(static_cast<const void*>(data), count * sizeof(charT));
		return *this;
	}

	template<typename charT, typename traits, typename Allocator>
	string& append(const std::basic_string<charT, traits, Allocator>& str) {
		this->append(str.data(), str.size() * sizeof(charT));
		return *this;
	}

	template<typename charT, typename traits, typename Allocator>
	string& append(const std::basic_string<charT, traits, Allocator>& str, size_t pos, size_t count) {
		if (pos >= str.size()) {
			throw std::out_of_range("util::string");
		}

		this->append(str.data() + pos, std::min(count, str.size() - pos) * sizeof(charT));
		return *this;
	}

	string& append(const void* data, size_t size);
	string& append(const util::buffer& buf);

	void reserve(size_t size);
	void clear();

	size_t size() const;
	size_t capacity() const;

	util::buffer buffer() const;
	operator util::buffer() const;

private:
	void append(size_t size);

	util::buffer _buffer;
	size_t _used;
};

} // namespace util

#endif // nodecc_util_string_h
