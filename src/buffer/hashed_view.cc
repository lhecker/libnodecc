#include "libnodecc/buffer.h"

#include <algorithm>


namespace node {

hashed_view& hashed_view::operator=(const buffer_view& other) {
	buffer_view::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_view& hashed_view::operator=(const hashed_view& other) {
	buffer_view::operator=(other);
	this->_hash = other._hash;
	return *this;
}

std::size_t hashed_view::hash() const noexcept {
	std::size_t hash = this->_hash;

	if (hash == 0) {
		hash = node::util::fnv1a<std::size_t>::hash(*this);

		if (hash == 0) {
			hash = 1;
		}

		const_cast<hashed_view*>(this)->_hash = hash;
	}

	return hash;
}

bool hashed_view::equals(const hashed_view& other) const noexcept {
	return this->data() && other.data()
	    && this->size() == other.size()
	    && this->hash() == other.hash()
	    && memcmp(this->data(), other.data(), this->size()) == 0;
}

} // namespace node
