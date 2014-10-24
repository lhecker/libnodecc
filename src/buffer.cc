#include "libnodecc/buffer.h"

#include <atomic>
#include <cstdlib>

#include "libnodecc/string.h"


namespace node {

struct buffer::control {
	constexpr control(void* base) noexcept : base(base), use_count(1) {};

	void* base;
	std::atomic<unsigned int> use_count;
};


buffer::buffer(const void* base, size_t size, node::flags flags) noexcept : buffer() {
	this->reset(base, size, flags);
}

buffer::buffer(buffer&& other) noexcept : _p(other._p), _data(other._data), _size(other._size) {
	other._p = nullptr;
	other._data = nullptr;
	other._size = 0;
}

buffer::buffer(const buffer& other) noexcept : _p(other._p), _data(other._data), _size(other._size) {
	this->retain();
}

buffer::buffer(node::string&& other) noexcept : _p(other._p), _data(other._data), _size(other._size) {
	other._p = nullptr;
	other._data = nullptr;
	other._size = 0;
	other._real_size = 0;
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

buffer& buffer::operator=(node::string&& other) noexcept {
	this->release();
	this->swap(other);

	return *this;
}

buffer::buffer(size_t size) noexcept : _p(nullptr) {
	this->reset(size);
}

buffer::~buffer() noexcept {
	this->release();
}

void buffer::swap(buffer& other) noexcept {
	std::swap(this->_p, other._p);
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
}

void buffer::swap(node::string& other) noexcept {
	std::swap(this->_p, other._p);
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
	other._real_size = other._size;
}

void buffer::reset() noexcept {
	this->release();
}

void buffer::reset(size_t size) noexcept {
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

void buffer::reset(const void* base, size_t size, node::flags flags) noexcept {
	this->release();

	this->_data = (void*)base;
	this->_size = size;

	switch (flags) {
	case node::flags::strong:
		this->_p = new control((void*)base);
		break;
	case node::flags::copy:
		this->copy(*this);
		break;
	default:
		;
	}
}

buffer buffer::copy(size_t size) const noexcept {
	buffer buffer;
	this->copy(buffer, size);
	return buffer;
}

buffer buffer::slice(ptrdiff_t start, ptrdiff_t end) const noexcept {
	buffer buffer;

	if (this->_size < size_t(PTRDIFF_MAX) && this->_data) {
		if (start < 0) {
			start += this->_size;

			if (start < 0) {
				start = 0;
			}
		} else if (size_t(start) > this->_size) {
			start = this->_size;
		}

		if (end < 0) {
			end += this->_size;

			if (end < 0) {
				end = 0;
			}
		} else if (size_t(end) > this->_size) {
			end = this->_size;
		}

		if (end >= start) {
			buffer._size = end - start;

			if (buffer._size > 0) {
				buffer._p = this->_p;
				buffer._data = this->get() + start;
				buffer.retain();
			}
		}
	}

	return buffer;
}

bool buffer::is_strong() const noexcept {
	return this->_p;
}

bool buffer::is_weak() const noexcept {
	return !this->_p;
}

buffer::operator void*() const noexcept {
	return this->data<void>();
}

buffer::operator char*() const noexcept {
	return this->data<char>();
}

buffer::operator unsigned char*() const noexcept {
	return this->data<unsigned char>();
}

char& buffer::operator[](size_t pos) const noexcept {
	return *(this->data<char>() + pos);
}

buffer::operator bool() const noexcept {
	return this->_data;
}

bool buffer::empty() const noexcept {
	return !this->_data;
}

size_t buffer::use_count() const noexcept {
	return this->_p ? this->_p->use_count.load(std::memory_order_relaxed) : 0;
}

uint8_t* buffer::get() const noexcept {
	return this->data<uint8_t>();
}

size_t buffer::size() const noexcept {
	return this->_size;
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

int buffer::compare(std::size_t size1, const void* data2, std::size_t size2) const noexcept {
	return this->compare(0, size1, data2, size2);
}

int buffer::compare(const void* data2, std::size_t size2) const noexcept {
	return this->compare(0, this->size(), data2, size2);
}

int buffer::compare(const buffer& other) const noexcept {
	return this->compare(0, this->size(), other.get(), other.size());
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


/*
 * Increasing the reference count is done using memory_order_relaxed,
 * since new references can only be formed from an existing reference and
 * passing an existing one already requires synchronization.
 */
void buffer::retain() noexcept {
	if (this->_p) {
		this->_p->use_count.fetch_add(1, std::memory_order_relaxed);
	}
}

void buffer::release() noexcept {
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

} // namespace node

bool operator==(const node::buffer& lhs, const node::buffer& rhs) noexcept {
	return lhs.data() == rhs.data();
}

bool operator!=(const node::buffer& lhs, const node::buffer& rhs) noexcept {
	return lhs.data() != rhs.data();
}
