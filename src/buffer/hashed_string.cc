#include "libnodecc/buffer.h"


namespace node {

hashed_string& hashed_string::operator=(const string& other) {
	string::operator=(other);
	this->_hash = 0;
	return *this;
}

hashed_string& hashed_string::operator=(const hashed_string& other) {
	string::operator=(other);
	this->_hash = other._hash;
	return *this;
}

std::size_t hashed_string::hash() const noexcept {
	std::size_t hash = this->_hash;

	if (hash == 0) {
		hash = node::util::fnv1a<std::size_t>::hash(*this);

		if (hash == 0) {
			hash = 1;
		}

		const_cast<hashed_string*>(this)->_hash = hash;
	}

	return hash;
}

bool hashed_string::equals(const hashed_string& other) const noexcept {
	return this->data() && other.data()
	    && this->size() == other.size()
	    && this->hash() == other.hash()
	    && memcmp(this->data(), other.data(), this->size()) == 0;
}

} // namespace node
