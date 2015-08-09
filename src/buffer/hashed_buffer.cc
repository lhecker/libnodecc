#include "libnodecc/buffer.h"


namespace node {

hashed_buffer& hashed_buffer::operator=(const buffer_view& other) {
	buffer::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_buffer& hashed_buffer::operator=(const buffer& other) {
	buffer::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_buffer& hashed_buffer::operator=(const hashed_buffer_view& other) {
	buffer::operator=(other);
	this->_hash = other.const_hash();
	return *this;
}

} // namespace node
