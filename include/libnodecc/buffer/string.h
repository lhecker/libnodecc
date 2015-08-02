#ifndef nodecc_buffer_string_h
#define nodecc_buffer_string_h

#include "buffer.h"


#define NODE_TO_STRING_MAP(XX)     \
	XX(int,                "%d")   \
	XX(long,               "%ld")  \
	XX(long long,          "%lld") \
	XX(unsigned,           "%u")   \
	XX(unsigned long,      "%lu")  \
	XX(unsigned long long, "%llu") \
	XX(float,              "%f")   \
	XX(double,             "%f")   \
	XX(long double,        "%Lf")  \


namespace node {

class string : public buffer {
public:
	using buffer::buffer;

	string() : buffer() {}

	string(const node::literal_string& other);
	string(const node::buffer_view& other);

	char* c_str() const noexcept {
		return this->data<char>();
	}
};


#define XX(_type_, _format_) string to_string(_type_ value);
NODE_TO_STRING_MAP(XX)
#undef XX

} // namespace node

#endif // nodecc_buffer_string_h
