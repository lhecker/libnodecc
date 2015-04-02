#include "libnodecc/buffer.h"

#include <atomic>
#include <cstdlib>

#include "libnodecc/util/math.h"


namespace node {

buffer_view::buffer_view(const void* data, std::size_t size) noexcept : _data(const_cast<void*>(data)), _size(size) {
}

buffer_view& buffer_view::operator=(const buffer_view& other) {
	_data = other._data;
	_size = other._size;
	return *this;
}



struct buffer::control {
	constexpr control(void* base) noexcept : base(base), use_count(1) {};

	void* base;
	std::atomic<uintptr_t> use_count;
};


buffer::buffer(const void* data, std::size_t size, node::flags flags) noexcept : buffer() {
	this->reset(data, size, flags);
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
	other._real_size = 0;
}

buffer::buffer(const buffer& other) noexcept : buffer_view(other._data, other._size), _p(other._p) {
	this->retain();
}

buffer& buffer::operator=(buffer&& other) noexcept {
	this->release();
	this->swap(other);

	return *this;
}

buffer& buffer::operator=(const buffer& other) noexcept {
	this->release();
	this->_p = other._p;
	this->_data = other._data;
	this->_size = other._size;
	this->retain();

	return *this;
}

buffer::buffer(std::size_t size) noexcept : _p(nullptr) {
	this->reset(size);
}

buffer::~buffer() noexcept {
	this->release();
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
	other._real_size = other._size;
}

void buffer::reset() noexcept {
	this->release();
}

void buffer::reset(std::size_t size) noexcept {
	this->release();

	if (size > 0) {
		uint8_t* base = (uint8_t*)malloc(sizeof(control) + size);
		uint8_t* data = base + sizeof(control);

		if (base) {
			this->_p = new(base) control(base);
			this->_data = data;
			this->_size = size;
		}
	}
}

void buffer::reset(const void* data, std::size_t size, node::flags flags) noexcept {
	this->release();

	this->_data = (void*)data;
	this->_size = size;

	switch (flags) {
	case node::flags::strong:
		this->_p = new control((void*)data);
		break;
	case node::flags::copy:
		this->copy(*this);
		break;
	default:
		;
	}
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

	if (PTRDIFF_GREATER_SIZE(PTRDIFF_MAX, this->_size) && this->_data) {
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
			buffer.retain();
		}
	}

#undef PTRDIFF_GREATER_SIZE

	return buffer;
}

void buffer::copy(buffer& target, std::size_t size) const noexcept {
	if (size == 0) {
		size = this->_size;
	}

	uint8_t* base = (uint8_t*)malloc(sizeof(control) + size);
	uint8_t* data = base + sizeof(control);

	// do this upfront in case of target == *this
	if (base && this->_data) {
		memcpy(data, this->_data, std::min(size, this->_size));
	}

	target.release();

	if (base) {
		target._p = new(base) control(base);
		target._data = data;
		target._size = size;
	}
}

int buffer::compare(std::size_t pos1, std::size_t size1, const void* data2, std::size_t size2) const noexcept {
	int r = 0;

	if (pos1 < this->size() && size1 <= (this->size() - pos1) && data2) {
		r = memcmp(this->get() + pos1, data2, std::min(size1, size2));

		if (r == 0) {
			r = size1 < size2 ? -1 : size1 > size2 ? 1 : 0;
		}
	}

	return r;
}

/*
 * Increasing the reference count is done using memory_order_relaxed,
 * since it doesn't matter in which order the count is increased
 * as long as the final sum in release() is correct.
 */
void buffer::retain() noexcept {
	if (this->_p) {
		this->_p->use_count.fetch_add(1, std::memory_order_relaxed);
	}
}

void buffer::release() noexcept {
	/*
	 * Normally std::memory_order_acq_rel should be used for the fetch_sub operation
	 * (to make all read/writes to the backing buffer visible before it's possibly freed),
	 * but this would result in an unneeded "acquire" operation, whenever the reference counter
	 * does not yet reach zero and thus may impose a performance penalty. Solution below:
	 */
	if (this->_p && this->_p->use_count.fetch_sub(1, std::memory_order_release) == 1) {
		std::atomic_thread_fence(std::memory_order_acquire);

		void* base = this->_p->base;
		free(base);

		if (this->_p != base) {
			delete this->_p;
		}
	}

	this->_p = nullptr;
	this->_data = nullptr;
	this->_size = 0;
}



mutable_buffer::mutable_buffer() noexcept : node::buffer(), _real_size(0) {
}

mutable_buffer::mutable_buffer(std::size_t size) noexcept : node::buffer(size), _real_size(0) {
	std::swap(this->_size, this->_real_size);
}

mutable_buffer::mutable_buffer(node::buffer&& other) noexcept : node::buffer(std::move(other)), _real_size(other._size) {
}

mutable_buffer& mutable_buffer::operator=(node::buffer&& other) noexcept {
	node::buffer::operator=(std::move(other));
	this->_real_size = other.size();
	return *this;
}

mutable_buffer::mutable_buffer(const node::buffer& other) noexcept : node::buffer(other), _real_size(other._size) {
}

mutable_buffer& mutable_buffer::operator=(const node::buffer& other) noexcept {
	node::buffer::operator=(other);
	this->_real_size = other._size;
	return *this;
}

mutable_buffer::mutable_buffer(mutable_buffer&& other) noexcept : node::buffer(std::move(other)), _real_size(other._real_size) {
}

mutable_buffer& mutable_buffer::operator=(mutable_buffer&& other) noexcept {
	node::buffer::operator=(std::move(other));
	this->_real_size = other._real_size;
	return *this;
}

mutable_buffer::mutable_buffer(const mutable_buffer& other) noexcept : node::buffer(other), _real_size(other._real_size) {
}

mutable_buffer& mutable_buffer::operator=(const mutable_buffer& other) noexcept {
	node::buffer::operator=(other);
	this->_real_size = other._real_size;
	return *this;
}

mutable_buffer& mutable_buffer::append(const void* data, std::size_t size) noexcept {
	this->_expand(size);
	memcpy(this->get() + this->_size, data, size);
	this->_size += size;
	return *this;
}

mutable_buffer& mutable_buffer::append(const node::buffer& buf, std::size_t pos, std::size_t count) noexcept {
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

mutable_buffer& mutable_buffer::append_number(std::size_t n, uint8_t base) {
	if (base >= 2 && base <= 36) {
		const std::size_t length = node::util::digits(n, base);
		std::size_t div = node::util::ipow(std::size_t(base), length - 1);

		this->_expand(length);

		do {
			static const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";

			const char c = chars[(n / div) % base];
			div /= base;

			*reinterpret_cast<char*>(this->get() + this->_size) = c;
			this->_size++;
		} while (div > 0);
	}

	return *this;
}

void mutable_buffer::reserve(std::size_t size) noexcept {
	if (size > this->capacity()) {
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
		std::size_t cap = this->capacity();
		cap = cap > (size + (size >> 1)) ? size : std::max(cap + (cap >> 1), size);

		std::size_t _size = this->_size;
		this->copy(*this, cap);
		this->_real_size = this->_size;
		this->_size = _size;
	}
}

void mutable_buffer::clear() noexcept {
	this->_size = 0;
}

void mutable_buffer::reset() noexcept {
	node::buffer::reset();
	this->_real_size = 0;
}

std::size_t mutable_buffer::capacity() const noexcept {
	return this->_real_size;
}

void mutable_buffer::expand_noinit(std::size_t size) noexcept {
	this->_expand(size);
	this->_size = this->_real_size;
}

void mutable_buffer::_expand(std::size_t size) noexcept {
	std::size_t cap = this->capacity();
	size += this->size();

	if (size < 16) {
		size = 16;
	}

	if (size > cap) {
		// see reserve(std::size_t) for details
		std::size_t _size = this->_size;
		this->copy(*this, std::max(cap + (cap >> 1), size));
		this->_real_size = this->_size;
		this->_size = _size;
	}
}

} // namespace node

bool operator==(const node::buffer& lhs, const node::buffer& rhs) noexcept {
	return lhs.data() == rhs.data();
}

bool operator!=(const node::buffer& lhs, const node::buffer& rhs) noexcept {
	return lhs.data() != rhs.data();
}
