#include "libnodecc/buffer.h"


namespace node {

hashed_buffer_view& hashed_buffer_view::operator=(const buffer_view& other) {
	buffer_view::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_buffer_view& hashed_buffer_view::operator=(const hashed_buffer_view& other) {
	buffer_view::operator=(other);
	this->_hash = other.const_hash();
	return *this;
}

} // namespace node
