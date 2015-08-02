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
	this->_hash = other._hash;
	return *this;
}

std::size_t hashed_buffer::hash() const noexcept {
	std::size_t hash = this->_hash;

	if (hash == 0) {
		hash = node::util::fnv1a<std::size_t>::hash(*this);

		if (hash == 0) {
			hash = 1;
		}

		const_cast<hashed_buffer*>(this)->_hash = hash;
	}

	return hash;
}

bool hashed_buffer::equals(const hashed_buffer_view& other) const noexcept {
	return this->data() && other.data()
	    && this->size() == other.size()
	    && this->hash() == other.hash()
	    && memcmp(this->data(), other.data(), this->size()) == 0;
}

} // namespace node
