#include "libnodecc/buffer.h"

#include "libnodecc/util/math.h"


namespace node {

mutable_buffer::mutable_buffer() noexcept : node::buffer(), _capacity(0) {
}

mutable_buffer::mutable_buffer(std::size_t capacity) noexcept : node::buffer(capacity), _capacity(0) {
	std::swap(this->_size, this->_capacity);
}

mutable_buffer::mutable_buffer(node::buffer&& other) noexcept : node::buffer(std::forward<node::buffer>(other)), _capacity(other._size) {
}

mutable_buffer& mutable_buffer::operator=(node::buffer&& other) noexcept {
	node::buffer::operator=(std::forward<node::buffer>(other));
	this->_capacity = other._size;
	return *this;
}

mutable_buffer::mutable_buffer(const node::buffer& other) noexcept : node::buffer(other), _capacity(other._size) {
}

mutable_buffer& mutable_buffer::operator=(const node::buffer& other) noexcept {
	node::buffer::operator=(other);
	this->_capacity = other._size;
	return *this;
}

mutable_buffer::mutable_buffer(mutable_buffer&& other) noexcept : node::buffer(std::forward<node::buffer>(other)), _capacity(other._capacity) {
}

mutable_buffer& mutable_buffer::operator=(mutable_buffer&& other) noexcept {
	node::buffer::operator=(std::forward<node::buffer>(other));
	this->_capacity = other._capacity;
	return *this;
}

mutable_buffer::mutable_buffer(const mutable_buffer& other) noexcept : node::buffer(other), _capacity(other._capacity) {
}

mutable_buffer& mutable_buffer::operator=(const mutable_buffer& other) noexcept {
	node::buffer::operator=(other);
	this->_capacity = other._capacity;
	return *this;
}

mutable_buffer& mutable_buffer::append(const void* data, std::size_t size) noexcept {
	void* p = this->_expand_size(size);

	if (p) {
		memcpy(p, data, size);
	}

	return *this;
}

mutable_buffer& mutable_buffer::append(const node::buffer& buf, std::size_t pos, std::size_t count) noexcept {
	if (pos < buf._size && count > 0) {
		if (count > buf._size - pos) {
			count = buf._size - pos;
		}

		if (this->_size > 0) {
			this->append(buf.data() + pos, count);
		} else {
			node::buffer::operator=(buf.slice(pos, count));
			this->_capacity = this->_size;
		}
	}

	return *this;
}

mutable_buffer& mutable_buffer::append_number(std::size_t n, uint8_t base) {
	if (base >= 2 && base <= 36) {
		const std::size_t length = node::util::digits(n, base);
		std::size_t div = node::util::ipow(std::size_t(base), length - 1);
		uint8_t* p = static_cast<uint8_t*>(this->_expand_size(length));

		if (p) {
			do {
				static const uint8_t chars[36] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

				*p++ = chars[(n / div) % base];
				div /= base;
			} while (div > 0);
		}
	}

	return *this;
}

std::size_t mutable_buffer::capacity() const noexcept {
	return this->_capacity;
}

void mutable_buffer::set_capacity(std::size_t capacity) noexcept {
	// we only want to change the _capacity --> preserve _size
	const std::size_t _size = this->_size;

	// after this _size will be "equal" to _capacity
	// (actually only nearly equal due to the exponential growth factor)
	this->set_size(capacity);

	// due to the previous call we need to recover the original _size
	// but only do so if the the buffer has grown compared to the original _size
	if (this->_size > _size) {
		this->_size = _size;
	}
}

void mutable_buffer::set_size(std::size_t size) noexcept {
	/*
	 * The growth rate should be exponential, that is that if we need to resize the buffer,
	 * we allocate more than what we need and thus prevent unneeded reallocations.
	 *
	 * The growth function after "n" resizes with a starting size of "T" is:
	 *   T*x^n <= T + T*x + T*x^2 + ... + T*x^(n-1)
	 * After removing T on both sides we get:
	 *   x^n <= 1 + x + x^2 + ... + x^(n-1)
	 *   x^n <= ∑x^i where i := 0..n-1
	 *
	 * Now we'd like the next allocation to always fit into the "hole"
	 * left by all the previous deallocations of the buffer memory
	 * (under optimal conditions those should be all lined up in memory).
	 *
	 * Thus the latter function above is only true for all x in (0,phi]
	 * (whith phi being the famous golden ratio).
	 *
	 * Here we use a growth rate of x = 1.5.
	 * If size is less than the "previous" capacity (e.g. divided by 1.5),
	 * then the allocation will be reset to exactly size,
	 * since it's smaller by a factor of over 1.5 (e.g. this buffer is much too big).
	 * If that's not the case either a growth rate of 1.5 is choosen, or if size
	 * is much larger than the current capacity (more than 1.5 times) size will be choosen.
	 */
	if (size > this->_capacity) {
		// if size is larger than cap
		this->copy(*this, std::max({ std::size_t(16), size, this->_capacity + (this->_capacity >> 1) }));
		this->_capacity = this->_size;
	} else if ((size + (size >> 1)) < this->_capacity) {
		// if size is much less than cap
		this->copy(*this, std::max(std::size_t(16), size));
		this->_capacity = this->_size;
	}

	this->_size = std::min(this->_capacity, size);
}

void mutable_buffer::clear() noexcept {
	this->_size = 0;
}

void* mutable_buffer::_expand_size(std::size_t size) {
	const std::size_t prev_size = this->size();

	size += prev_size;

	if (size > this->_capacity) {
		this->copy(*this, std::max({ std::size_t(16), size, this->_capacity + (this->_capacity >> 1) }));

		if (!this->_size) {
			return nullptr;
		}

		this->_capacity = this->_size;
	}

	this->_size = size;

	return this->data() + prev_size;
}

} // namespace node
