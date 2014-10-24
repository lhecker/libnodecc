#include "libnodecc/string.h"


namespace node {

string::string() noexcept : node::buffer(), _real_size(0) {
}

string::string(size_t size) noexcept : node::buffer(size), _real_size(0) {
	std::swap(this->_size, this->_real_size);
}

string::string(node::buffer&& other) noexcept : node::buffer(std::move(other)), _real_size(other._size) {
}

string& string::operator=(node::buffer&& other) noexcept {
	node::buffer::operator=(std::move(other));
	this->_real_size = other.size();
	return *this;
}

string::string(const node::buffer& other) noexcept : node::buffer(other), _real_size(other._size) {
}

string& string::operator=(const node::buffer& other) noexcept {
	node::buffer::operator=(other);
	this->_real_size = other._size;
	return *this;
}

string::string(string&& other) noexcept : node::buffer(std::move(other)), _real_size(other._real_size) {
}

string& string::operator=(string&& other) noexcept {
	node::buffer::operator=(std::move(other));
	this->_real_size = other._real_size;
	return *this;
}

string::string(const string& other) noexcept : node::buffer(other), _real_size(other._real_size) {
}

string& string::operator=(const string& other) noexcept {
	node::buffer::operator=(other);
	this->_real_size = other._real_size;
	return *this;
}

string& string::append(const void* data, size_t size) noexcept {
	this->append(size);
	memcpy(this->get() + this->_size, data, size);
	this->_size += size;
	return *this;
}

string& string::append(const node::buffer& buf, size_t pos, size_t count) noexcept {
	if (pos < buf.size() && count > 0) {
		if (count > buf.size() - pos) {
			count = buf.size() - pos;
		}

		if (this->_size > 0) {
			this->append(buf.data() + pos, count);
		} else {
			node::buffer::operator=(buf.slice(pos, count));
			this->_real_size = this->_size;
		}
	}

	return *this;
}

void string::reserve(size_t size) noexcept {
	if (size > this->capacity()) {
		/*
		 * The growth rate should be exponential, that is that if we need to resize,
		 * we allocate more that we need and thus prevent reallocations at every small append.
		 *
		 * Thus the growth function after n resizes with the starting size T is:
		 *   T*x^n <= T + T*x + T*x^2 + ... + T*x^(n-1)
		 * After removing T on both sides we get:
		 *   x^n <= 1 + x + x^2 + ... + x^(n-1)
		 *   x^n <= ∑x^i where i := 0..n-1
		 *
		 * Now we want that the next allocation always fits into the "holes"
		 * left by the deallocation of all previous allocations
		 * (under the optimal condition where those are all lined up in memory).
		 *
		 * Thus the latter function above is only true for all x in (0,phi]
		 * (whith phi being the famous golden ratio).
		 *
		 * Here we use a growth rate of x = 1.5.
		 * If size is less than the "previous" capacity (e.g. divided by 1.5),
		 * then the allocation will be reset to exactly size,
		 * since it's smaller by a factor of over 1.5 and thus this buffer much too large.
		 * If that's not the case either a growth rate of 1.5 is choosen, or if size
		 * is much larger than the current capacity (more than 1.5 times) size will be choosen.
		 */
		size_t cap = this->capacity();
		cap = cap > (size + (size >> 1)) ? size : std::max(cap + (cap >> 1), size);

		size_t _size = this->_size;
		this->copy(*this, cap);
		this->_real_size = this->_size;
		this->_size = _size;
	}
}

void string::clear() noexcept {
	this->reset();
	this->_real_size = 0;
}

size_t string::capacity() const noexcept {
	return this->_real_size;
}

void string::append(size_t size) noexcept {
	size_t cap = this->capacity();
	size += this->size();

	if (size < 16) {
		size = 16;
	}

	if (size > cap) {
		// see reserve(size_t) for details
		size_t _size = this->_size;
		this->copy(*this, std::max(cap + (cap >> 1), size));
		this->_real_size = this->_size;
		this->_size = _size;
	}
}

} // namespace node
