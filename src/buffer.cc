#include "libnodecc/buffer.h"

#include <atomic>
#include <cstdlib>

#include "libnodecc/util/math.h"


typedef void(*free_signature)(void*);


namespace node {

buffer_view::buffer_view(const void* data, std::size_t size) noexcept : _data(const_cast<void*>(data)), _size(size) {
}

buffer_view& buffer_view::operator=(const buffer_view& other) {
	_data = other._data;
	_size = other._size;
	return *this;
}


buffer::buffer(std::size_t size) noexcept : buffer() {
	this->_reset_unsafe(size);
}

buffer::buffer(const void* data, std::size_t size, buffer_flags flags) noexcept : buffer_view(data, size), _p(nullptr) {
	if (flags == node::copy) {
		this->copy(*this);
	}
}

buffer::buffer(buffer&& other) noexcept : buffer_view(other._data, other._size), _p(other._p) {
	other._p = nullptr;
	other._data = nullptr;
	other._size = 0;
}

buffer::buffer(mutable_buffer&& other) noexcept : buffer_view(other._data, other._size), _p(other._p) {
	other._p = nullptr;
	other._data = nullptr;
	other._size = 0;
	other._capacity = 0;
}

buffer::buffer(const buffer& other) noexcept : buffer_view(other._data, other._size), _p(other._p) {
	this->_retain();
}

buffer& buffer::operator=(buffer&& other) noexcept {
	this->_release();
	this->swap(other);

	return *this;
}

buffer& buffer::operator=(const buffer& other) noexcept {
	this->_release();
	this->_p = other._p;
	this->_data = other._data;
	this->_size = other._size;
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
	std::swap(this->_p, other._p);
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
}

void buffer::swap(mutable_buffer& other) noexcept {
	std::swap(this->_p, other._p);
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	other._capacity = other._size;
}

void buffer::reset() noexcept {
	this->_release();
}

void buffer::reset(std::size_t size) noexcept {
	this->_release();
	this->_reset_unsafe(size);
}

void buffer::reset(const void* data, std::size_t size, buffer_flags flags) noexcept {
	this->_release();

	this->_data = (void*)data;
	this->_size = size;

	if (flags == node::copy) {
		this->copy(*this);
	}
}

void buffer::reset(const buffer_view other, buffer_flags flags) noexcept {
	this->reset(other.data(), other.size(), flags);
}

void buffer::reset(const char str[], buffer_flags flags) noexcept {
	this->reset(str, strlen(str), flags);
}

buffer buffer::copy(std::size_t size) const noexcept {
	buffer buffer;
	this->copy(buffer, size);
	return buffer;
}

buffer buffer::slice(std::ptrdiff_t start, std::ptrdiff_t end) const noexcept {
	buffer buffer;

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
			buffer._size = end - start;
			buffer._p = this->_p;
			buffer._data = this->get() + start;
			buffer._retain();
		}
	}

#undef PTRDIFF_GREATER_SIZE

	return buffer;
}

void buffer::copy(buffer& target, std::size_t size) const noexcept {
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

int buffer::compare(std::size_t pos1, std::size_t size1, const void* data2, std::size_t size2) const noexcept {
	int r = 0;

	if (pos1 < this->_size && size1 <= (this->_size - pos1) && data2) {
		r = memcmp(this->get() + pos1, data2, std::min(size1, size2));

		if (r == 0) {
			r = size1 < size2 ? -1 : size1 > size2 ? 1 : 0;
		}
	}

	return r;
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
	control_base* p = this->_p;

	if (p) {
		p->use_count.fetch_add(1, std::memory_order_relaxed);
	}
}

void buffer::_release() noexcept {
	/*
	 * Normally std::memory_order_acq_rel should be used for the fetch_sub operation
	 * (to make all read/writes to the backing buffer visible before it's possibly freed),
	 * but this would result in an unneeded "acquire" operation, whenever the reference counter
	 * does not yet reach zero and thus may impose a performance penalty. Solution below:
	 */
	control_base* p = this->_p;

	if (p && p->use_count.fetch_sub(1, std::memory_order_release) == 1) {
		std::atomic_thread_fence(std::memory_order_acquire);

		const void* base = p->base;

		p->free();

		if (p != base) {
			delete p;
		}
	}

	this->_p = nullptr;
	this->_data = nullptr;
	this->_size = 0;
}



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

void mutable_buffer::reset() noexcept {
	node::buffer::reset();
	this->_capacity = 0;
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

bool operator==(const node::buffer_view& lhs, const node::buffer_view& rhs) noexcept {
	return lhs.data() == rhs.data();
}

bool operator!=(const node::buffer_view& lhs, const node::buffer_view& rhs) noexcept {
	return lhs.data() != rhs.data();
}
