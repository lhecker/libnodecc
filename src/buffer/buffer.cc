#include "libnodecc/buffer.h"

#include <algorithm>
#include <cstdlib>


namespace node {

buffer::buffer(std::size_t size) : buffer() {
	this->_reset_unsafe(size);
}

buffer::buffer(const buffer& other) noexcept : buffer_view(other), _p(other._p) {
	this->_retain();
}

buffer::buffer(buffer&& other) noexcept : buffer_view(other), _p(other._p) {
	other._reset_zero();
}

buffer::buffer(mutable_buffer&& other) noexcept : buffer_view(other), _p(other._p) {
	other._reset_zero();
}

buffer::buffer(const void* data, std::size_t size, buffer_flags flags) noexcept : buffer_view(data, size), _p(nullptr) {
	if (flags == buffer_flags::copy) {
		this->_copy(*this);
	}
}

buffer& buffer::operator=(const buffer_view& other) {
	this->_release();
	buffer_view::operator=(other);

	return *this;
}

buffer& buffer::operator=(buffer&& other) {
	this->_release();
	this->swap(other);

	return *this;
}

buffer& buffer::operator=(const buffer& other) {
	this->_release();
	this->_data = other._data;
	this->_size = other._size;
	this->_p = other._p;
	this->_retain();

	return *this;
}

buffer::~buffer() {
	this->_release();
}

std::size_t buffer::use_count() const noexcept {
	return this->_p ? this->_p->use_count.load(std::memory_order_acquire) : 0;
}

void buffer::swap(buffer& other) noexcept {
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	std::swap(this->_p, other._p);
}

void buffer::swap(mutable_buffer& other) noexcept {
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	std::swap(this->_p, other._p);

	other._capacity = other._size;
}

void buffer::reset() {
	this->_release();
}

void buffer::reset(std::size_t size) {
	this->_release();
	this->_reset_unsafe(size);
}

void buffer::reset(const buffer_view& other, buffer_flags flags) {
	this->_release();

	this->_data = other.data();
	this->_size = other.size();

	if (flags == buffer_flags::copy) {
		this->_copy(*this);
	}
}

buffer buffer::copy(std::size_t size) const {
	buffer buf;
	this->_copy(buf, size);
	return buf;
}

buffer buffer::slice(std::size_t beg, std::size_t end) const noexcept {
	buffer buf;

	if (this->_data) {
		const std::size_t cbeg = std::min(beg, this->size());
		const std::size_t cend = std::min(end, this->size());

		if (cend > cbeg) {
			buf._data = this->data() + cbeg;
			buf._size = cend - cbeg;
			buf._p = this->_p;
			buf._retain();
		}
	}

#undef PTRDIFF_GREATER_SIZE

	return buf;
}

void buffer::_copy(buffer& target, std::size_t size) const {
	if (size == 0) {
		size = this->_size;
	}

	buffer buf(size);

	if (this->_data) {
		memcpy(buf._data, this->_data, std::min(size, this->_size));
	}

	target = std::move(buf);
}

void buffer::_reset_unsafe(std::size_t size) {
	if (size > 0) {
		constexpr const std::size_t control_size = (sizeof(default_control) + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
		uint8_t* base = (uint8_t*)malloc(control_size + size);
		uint8_t* data = base + control_size;

		if (base) {
			this->_p = new(base) default_control(base);
			this->_data = data;
			this->_size = size;
		}
	}
}

void buffer::_reset_zero() noexcept {
	this->_data = nullptr;
	this->_size = 0;
	this->_p = nullptr;
}

/*
 * Increasing the reference count is done using memory_order_relaxed,
 * since it doesn't matter in which order the count is increased
 * as long as the final sum in _release() is correct.
 */
void buffer::_retain() noexcept {
	if (this->_p) {
		this->_p->retain();
	}
}

void buffer::_release() {
	if (this->_p) {
		this->_p->release();
	}

	this->_reset_zero();
}

/*
 * Increasing the reference count is done using memory_order_relaxed,
 * since it doesn't matter in which order the count is increased
 * as long as the final sum in _release() is correct.
 */
void buffer::control_base::retain() noexcept {
	this->use_count.fetch_add(1, std::memory_order_relaxed);
}

void buffer::control_base::release() {
	/*
	 * Normally std::memory_order_acq_rel should be used for the fetch_sub operation
	 * (to make all read/writes to the backing buffer visible before it's possibly freed),
	 * but this would result in an unneeded "acquire" operation, whenever the reference counter
	 * does not yet reach zero and thus may impose a performance penalty. Solution below:
	 */
	if (this->use_count.fetch_sub(1, std::memory_order_release) == 1) {
		std::atomic_thread_fence(std::memory_order_acquire);

		const void* base = this->base;
		const bool is_default_control_type = base == this;

		if (is_default_control_type) {
			this->~control_base();
			std::free(const_cast<void*>(base));
		} else {
			this->free();
			delete this;
		}
	}
}

} // namespace node
