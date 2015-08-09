#include "libnodecc/buffer.h"


namespace node {

hashed_string& hashed_string::operator=(const string& other) {
	string::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_string& hashed_string::operator=(const hashed_string& other) {
	string::operator=(other);
	this->_hash = other.const_hash();
	return *this;
}

} // namespace node
