#ifndef nodecc_buffer_string_h
#define nodecc_buffer_string_h

#include "buffer.h"

#include <cassert>


namespace node {

class string : public buffer {
	template<typename Value, std::size_t Size>
	friend string to_string_impl(const char format[], Value v);

public:
	/*
	 * TODO: store a reference to control_base in the deleter
	 * ---> Create "control_base_ptr" which wraps a control_base and calls retain()/release().
	 *      node::buffer should simply inherit (multiple inheritance) from it.
	 */
	struct c_str_deleter {
		void operator()(const char* str) const noexcept {}
	};


	using buffer::buffer;

	string() : buffer() {}

	string(const node::literal_string& other);
	string(const node::buffer_view& other);

	explicit string(std::size_t size);
	explicit string(const void* data, std::size_t size);

	template<typename CharT>
	string(const CharT* str) : string(const_cast<CharT*>(str), std::char_traits<CharT>::length(str) * sizeof(CharT)) {}

	void reset();
	void reset(std::size_t size);

	string copy(std::size_t size) const;

	std::unique_ptr<const char, c_str_deleter> create_c_str() const noexcept;
	const char* c_str() const noexcept;

private:
	void _copy(string& target, std::size_t size = 0) const;
	void _reset_unsafe(std::size_t size);
};


string to_string(int val);
string to_string(long int val);
string to_string(long long int val);

string to_string(unsigned int val);
string to_string(unsigned long int val);
string to_string(unsigned long long int val);

string to_string(float val);
string to_string(double val);
string to_string(long double val);

} // namespace node

#endif // nodecc_buffer_string_h
