#include "libnodecc/util/string.h"


util::string& util::string::append(const void* data, size_t size) {
	this->append(size);
	memcpy(this->get() + this->_used, data, size);
	this->_used += size;
	return *this;
}

util::string& util::string::append(const util::buffer& buf) {
	this->append(buf.data(), buf.size());
	return *this;
}

void util::string::reserve(size_t size) {
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
		this->copy(*this, cap);
	}
}

void util::string::clear() {
	this->reset();
	this->_used = 0;
}

size_t util::string::size() const {
	return this->_used;
}

size_t util::string::capacity() const {
	return this->size();
}

void util::string::append(size_t size) {
	size_t cap = this->capacity();
	size += this->size();

	if (size > cap) {
		// see reserve(size_t) for details
		this->copy(*this, std::max(cap + (cap >> 1), size));
	}
}
