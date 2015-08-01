#include "libnodecc/buffer.h"


namespace node {

hashed_buffer_view& hashed_buffer_view::operator=(const buffer_view& other) {
	buffer_view::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_buffer_view& hashed_buffer_view::operator=(const hashed_buffer_view& other) {
	buffer_view::operator=(other);
	this->_hash = other._hash;
	return *this;
}

std::size_t hashed_buffer_view::hash() const noexcept {
	std::size_t hash = this->_hash;

	if (hash == 0) {
		hash = node::util::fnv1a<std::size_t>::hash(*this);

		if (hash == 0) {
			hash = 1;
		}

		const_cast<hashed_buffer_view*>(this)->_hash = hash;
	}

	return hash;
}

bool hashed_buffer_view::equals(const hashed_buffer_view& other) const noexcept {
	return this->data() && other.data()
	    && this->size() == other.size()
	    && this->hash() == other.hash()
	    && memcmp(this->data(), other.data(), this->size()) == 0;
}

} // namespace node
