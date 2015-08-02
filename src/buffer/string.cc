#include "libnodecc/buffer.h"

#include "libnodecc/util/math.h"


namespace node {

string::string(const node::literal_string& other) : buffer(other) {
}

string::string(const node::buffer_view& other) : buffer(other.size() ? other.size() + 1 : 0) {
	if (other.size()) {
		memcpy(this->data(), other.data(), other.size());
		this->data<char>()[other.size()] = '\0';
	}
}


#define XX(_type_, _format_)                                                   \
	string to_string(_type_ value) {                                           \
		string buf(node::util::digits(value, 10) + 1);                         \
		const int r = snprintf(buf.data<char>(), buf.size(), _format_, value); \
		                                                                       \
		if (r <= 0) {                                                          \
			buf.reset();                                                       \
		}                                                                      \
		                                                                       \
		return buf;                                                            \
	}                                                                          \

NODE_TO_STRING_MAP(XX)
#undef XX

} // namespace node
