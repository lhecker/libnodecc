#include "libnodecc/buffer.h"

#include <algorithm>
#include <cstdlib>


typedef void(*free_signature)(void*);


namespace node {

buffer::buffer(std::size_t size) noexcept : buffer() {
	this->_reset_unsafe(size);
}

buffer::buffer(const void* data, std::size_t size, buffer_flags flags) noexcept : buffer_view(data, size), _p(nullptr) {
	if (flags == node::copy) {
		this->copy(*this);
	}
}

buffer::buffer(buffer&& other) noexcept : buffer_view(other), _p(other._p) {
	memset(&other, 0, sizeof(other));
}

buffer::buffer(mutable_buffer&& other) noexcept : buffer_view(other), _p(other._p) {
	memset(&other, 0, sizeof(other));
}

buffer::buffer(const buffer& other) noexcept : buffer_view(other), _p(other._p) {
	this->_retain();
}

buffer& buffer::operator=(buffer_view&& other) noexcept {
	this->_release();

	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	std::swap(this->_hash, other._hash);

	return *this;
}

buffer& buffer::operator=(const buffer_view& other) noexcept {
	this->_release();

	this->_data = other._data;
	this->_size = other._size;
	this->_hash = other._hash;

	return *this;
}

buffer& buffer::operator=(buffer&& other) noexcept {
	this->_release();
	this->swap(other);

	return *this;
}

buffer& buffer::operator=(const buffer& other) noexcept {
	this->_release();
	this->_data = other._data;
	this->_size = other._size;
	this->_hash = other._hash;
	this->_p = other._p;
	this->_retain();

	return *this;
}

buffer::~buffer() noexcept {
	this->_release();
}

std::size_t buffer::use_count() const noexcept {
	return this->_p ? this->_p->use_count.load(std::memory_order_acquire) : 0;
}

void buffer::swap(buffer& other) noexcept {
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	std::swap(this->_hash, other._hash);
	std::swap(this->_p, other._p);
}

void buffer::swap(mutable_buffer& other) noexcept {
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	std::swap(this->_hash, other._hash);
	std::swap(this->_p, other._p);

	other._capacity = other._size;
}

void buffer::reset() noexcept {
	this->_release();
}

void buffer::reset(std::size_t size) noexcept {
	this->_release();
	this->_reset_unsafe(size);
}

void buffer::reset(const buffer_view& other, buffer_flags flags) noexcept {
	this->_release();

	this->_data = other._data;
	this->_size = other._size;
	this->_hash = other._hash;

	if (flags == node::copy) {
		this->copy(*this);
	}
}

void buffer::reset(const void* data, std::size_t size, buffer_flags flags) noexcept {
	this->reset(buffer_view(data, size), flags);
}

void buffer::reset(const char str[], buffer_flags flags) noexcept {
	this->reset(buffer_view(str), flags);
}

buffer buffer::copy(std::size_t size) const noexcept {
	buffer buf;
	this->_copy(buf, size);
	return buf;
}

buffer buffer::slice(std::ptrdiff_t start, std::ptrdiff_t end) const noexcept {
	buffer buf;

#if PTRDIFF_MAX > SIZE_T_MAX
# define PTRDIFF_GREATER_SIZE(ptrdiff, size) ((ptrdiff) > std::ptrdiff_t(size))
#else
# define PTRDIFF_GREATER_SIZE(ptrdiff, size) (std::size_t(ptrdiff) > (size))
#endif

	if (this->_data && PTRDIFF_GREATER_SIZE(PTRDIFF_MAX, this->_size)) {
		if (start < 0) {
			start += this->_size;

			if (start < 0) {
				start = 0;
			}
		} else if (PTRDIFF_GREATER_SIZE(start, this->_size)) {
			start = this->_size;
		}

		if (end < 0) {
			end += this->_size;

			if (end < 0) {
				end = 0;
			}
		} else if (PTRDIFF_GREATER_SIZE(end, this->_size)) {
			end = this->_size;
		}

		if (end > start) {
			buf._data = this->data() + start;
			buf._size = end - start;
			buf._p = this->_p;
			buf._retain();
		}
	}

#undef PTRDIFF_GREATER_SIZE

	return buf;
}

void buffer::_copy(buffer& target, std::size_t size) const noexcept {
	if (size == 0) {
		size = this->_size;
	}

	constexpr const std::size_t control_size = (sizeof(control<free_signature>) + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
	uint8_t* base = (uint8_t*)malloc(control_size + size);
	uint8_t* data = base + control_size;

	// we need to do this upfront since target might be *this
	if (base && this->_data) {
		memcpy(data, this->_data, std::min(size, this->_size));
	}

	target._release();

	if (base) {
		target._p = new(base) control<free_signature>(base, free);
		target._data = data;
		target._size = size;
	}
}

void buffer::_reset_zero() noexcept {
	this->_data = nullptr;
	this->_size = 0;
	this->_hash = 0;
	this->_p = nullptr;
}

void buffer::_reset_unsafe(std::size_t size) noexcept {
	if (size > 0) {
		constexpr const std::size_t control_size = (sizeof(control<free_signature>) + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
		uint8_t* base = (uint8_t*)malloc(control_size + size);
		uint8_t* data = base + control_size;

		if (base) {
			this->_p = new(base) control<free_signature>(base, free);
			this->_data = data;
			this->_size = size;
		}
	}
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

void buffer::_release() noexcept {
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

void buffer::control_base::release() noexcept {
	/*
	 * Normally std::memory_order_acq_rel should be used for the fetch_sub operation
	 * (to make all read/writes to the backing buffer visible before it's possibly freed),
	 * but this would result in an unneeded "acquire" operation, whenever the reference counter
	 * does not yet reach zero and thus may impose a performance penalty. Solution below:
	 */
	if (this->use_count.fetch_sub(1, std::memory_order_release) == 1) {
		std::atomic_thread_fence(std::memory_order_acquire);

		const void* base = this->base;

		this->free();

		/*
		 * Remember the Special Mode:
		 *  If (base == this) then control and base
		 *  have been allocated using a single malloc().
		 */
		if (base != this) {
			delete this;
		}
	}
}

} // namespace node
