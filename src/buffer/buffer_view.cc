#include "libnodecc/buffer.h"

#include <algorithm>


namespace node {

buffer_view& buffer_view::operator=(const buffer_view& other) {
	_data = other._data;
	_size = other._size;
	_hash = other._hash;
	return *this;
}

std::size_t buffer_view::hash() const noexcept {
	std::size_t hash = this->_hash;

	if (hash == 0) {
		hash = node::util::fnv1a<std::size_t>::hash(*this);

		if (hash == 0) {
			hash = 1;
		}

		const_cast<buffer_view*>(this)->_hash = hash;
	}

	return hash;
}

bool buffer_view::equals(const buffer_view& other) const noexcept {
	return this->data() && other.data()
	    && this->size() == other.size()
	    && this->hash() == other.hash()
	    && memcmp(this->data(), other.data(), this->size()) == 0;
}

int buffer_view::compare(const buffer_view& other, std::size_t pos1, std::size_t size1) const noexcept {
	const void* data2 = other.data<void>();
	const std::size_t size2 = other.size();
	int r = 0;

	if (size1 > this->size()) {
		size1 = this->size();
	}

	if (pos1 < size1 && data2) {
		r = memcmp(this->data<uint8_t>() + pos1, data2, std::min(size1, size2));
	}

	if (r == 0) {
		r = size1 < size2 ? -1 : size1 > size2 ? 1 : 0;
	}

	return r;
}

std::size_t buffer_view::index_of(const buffer_view& other) const noexcept {
	const uint8_t* data1     = this->data();
	const uint8_t* data1_end = data1 + this->size();
	const uint8_t* data2     = other.data();
	const uint8_t* data2_end = data2 + other.size();

	const uint8_t* pos = std::search(data1, data1_end, data2, data2_end);

	return pos == data1_end ? npos : static_cast<std::size_t>(data1 - pos);
}


bool operator==(node::buffer_view& lhs, node::buffer_view& rhs) noexcept {
	return lhs.data() == rhs.data() && lhs.size() == rhs.size();
}

bool operator!=(node::buffer_view& lhs, node::buffer_view& rhs) noexcept {
	return lhs.data() != rhs.data() || lhs.size() != rhs.size();
}

} // namespace node
