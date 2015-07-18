#include "libnodecc/buffer.h"


namespace node {

buffer_view& buffer_view::operator=(const buffer_view& other) {
	_data = other._data;
	_size = other._size;
	_hash = other._hash;
	return *this;
}

std::size_t buffer_view::hash() noexcept {
	if (this->_hash == 0) {
		this->_hash = node::util::fnv1a(this->data<uint8_t>(), this->_size);

		if (this->_hash == 0) {
			this->_hash = 1;
		}
	}

	return this->_hash;
}

int buffer_view::compare(std::size_t pos1, std::size_t size1, const void* data2, std::size_t size2) const noexcept {
	int r = 0;

	// this->_size must be greater than 0 and thus data1 must be non-null
	if (pos1 < this->_size && size1 <= (this->_size - pos1) && data2) {
		r = memcmp(this->data<uint8_t>() + pos1, data2, std::min(size1, size2));
	}

	if (r == 0) {
		r = size1 < size2 ? -1 : size1 > size2 ? 1 : 0;
	}

	return r;
}


bool operator==(node::buffer_view& lhs, node::buffer_view& rhs) noexcept {
	return lhs.hash() == rhs.hash() && lhs.compare(rhs) == 0;
}

bool operator!=(node::buffer_view& lhs, node::buffer_view& rhs) noexcept {
	return lhs.hash() != rhs.hash() || lhs.compare(rhs) != 0;
}

} // namespace node
