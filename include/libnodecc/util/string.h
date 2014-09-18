#ifndef nodecc_util_string_h
#define nodecc_util_string_h

#include "buffer.h"


namespace util {

class string : public util::buffer {
public:
	explicit string() noexcept;
	explicit string(size_t size) noexcept;

	string(util::string&& other) noexcept;
	string(const util::string& other) noexcept;
	util::string& operator=(const util::string& other) noexcept;

	string& append(const void* data, size_t size) noexcept;
	string& append(const util::buffer& buf, size_t pos = 0, size_t count = SIZE_T_MAX) noexcept;

	template<typename charT>
	void push_back(charT ch) noexcept {
		this->append(sizeof(charT));
		*reinterpret_cast<charT*>(this->get() + this->_size) = ch;
		this->_size += sizeof(charT);
	}

	template<typename charT>
	string& append(const charT* data) noexcept {
		this->append(static_cast<const void*>(data), strlen(data));
		return *this;
	}

	template<typename charT>
	string& append(const charT* data, size_t count) noexcept {
		this->append(static_cast<const void*>(data), count * sizeof(charT));
		return *this;
	}

	template<typename charT, typename traits, typename Allocator>
	string& append(const std::basic_string<charT, traits, Allocator>& str) noexcept {
		this->append(str.data(), str.size() * sizeof(charT));
		return *this;
	}

	template<typename charT, typename traits, typename Allocator>
	string& append(const std::basic_string<charT, traits, Allocator>& str, size_t pos, size_t count) noexcept {
		if (pos >= str.size()) {
			throw std::out_of_range("util::string");
		}

		this->append(str.data() + pos, std::min(count, str.size() - pos) * sizeof(charT));
		return *this;
	}

	void reserve(size_t size) noexcept;
	void clear() noexcept;

	size_t capacity() const noexcept;

private:
	void append(size_t size) noexcept;

	size_t _real_size;
};

} // namespace util

#endif // nodecc_util_string_h
