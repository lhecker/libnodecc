#include "libnodecc/buffer.h"


namespace node {

buffer_view::buffer_view(const void* data, std::size_t size) noexcept : _data(const_cast<void*>(data)), _size(size) {
}

buffer_view& buffer_view::operator=(const buffer_view& other) {
	_data = other._data;
	_size = other._size;
	return *this;
}


bool operator==(const node::buffer_view& lhs, const node::buffer_view& rhs) noexcept {
	return lhs.data() == rhs.data();
}

bool operator!=(const node::buffer_view& lhs, const node::buffer_view& rhs) noexcept {
	return lhs.data() != rhs.data();
}

} // namespace node
