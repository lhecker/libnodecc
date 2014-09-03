#include "libnodecc/util/buffer.h"

#include <atomic>
#include <cstdlib>


namespace util {

struct buffer::control {
	constexpr control(void *base) noexcept : base(base), use_count(1) {};

	void *base;
	std::atomic<unsigned int> use_count;
};

}


util::buffer::buffer(const void *base, size_t size, util::flags flags) noexcept : _p(nullptr) {
	this->reset(base, size, flags);
}

util::buffer::buffer(const buffer &other) noexcept : _p(other._p), _data(other._data), _size(other._size) {
	this->retain();
}

util::buffer& util::buffer::operator=(const buffer &other) noexcept {
	this->release();
	this->_p = other._p;
	this->_data = other._data;
	this->_size = other._size;
	this->retain();

	return *this;
}

util::buffer::buffer(size_t size) noexcept : buffer() {
	this->copy(size);
}

util::buffer::~buffer() noexcept {
	this->release();
}

void util::buffer::reset() noexcept {
	this->_p = nullptr;
	this->_data = nullptr;
	this->_size = 0;
}

void util::buffer::reset(const void *base, size_t size, util::flags flags) noexcept {
	this->release();

	this->_data = (uint8_t*)base;
	this->_size = size;

	switch (flags) {
		case util::flags::strong:
			this->_p = new control((void*)base);
		case util::flags::copy:
			this->copy();
		default:
			;
	}
}

size_t util::buffer::use_count() const noexcept {
	return this->_p ? this->_p->use_count.load(std::memory_order_relaxed) : 0;
}

bool util::buffer::is_strong() const noexcept {
	return this->_p;
}

bool util::buffer::is_weak() const noexcept {
	return !this->_p;
}

util::buffer::operator bool() const noexcept {
	return this->_p != nullptr;
}

util::buffer::operator void*() const noexcept {
	return this->data<void>();
}

util::buffer::operator char*() const noexcept {
	return this->data<char>();
}

util::buffer::operator unsigned char*() const noexcept {
	return this->data<unsigned char>();
}

uint8_t *util::buffer::get() const noexcept {
	return this->data<uint8_t>();
}

size_t util::buffer::size() const noexcept {
	return this->_size;
}

bool util::buffer::copy(bool copy, size_t size) noexcept {
	if (size == 0) {
		size = this->size();
	}

	uint8_t *base = (uint8_t*)malloc(sizeof(control) + size);
	uint8_t *data = base + sizeof(control);

	if (!base) {
		return false;
	}

	if (copy && this->_data) {
		memcpy(data, this->data<void>(), std::min(this->size(), size));
	}

	this->release();

	this->_p = new(base) control(base);
	this->_data = data;
	this->_size = size;

	return true;
}

util::buffer util::buffer::slice(size_t offset, size_t size) const noexcept {
	util::buffer buffer;
	buffer._p = this->_p;

	if (this->_data) {
		/*
		 * If offset/size are too large the buffer would refer
		 * to memory outside of the original one.
		 * In that case or if size is the default value of zero,
		 * we default to the maximum possible value
		 */
		if (offset > this->size()) {
			offset = this->size();
			size = 0;
		} else if (size == 0 || offset + size > this->size()) {
			size = this->size() - offset;
		}

		buffer._data = this->data<uint8_t>() + offset;
		buffer._size = size;
	}

	return buffer;
}

util::buffer util::buffer::slice(const void *data, size_t size) const noexcept {
	size_t offset = SIZE_T_MAX;

	if (data >= this->_data) {
		offset = (uint8_t*)data - this->data<uint8_t>();
	}

	return this->slice(offset, size);
}

/*
 * Increasing the reference count is done using memory_order_relaxed,
 * since new references can only be formed from an existing reference and
 * passing an existing one already requires synchronization.
 */
void util::buffer::retain() noexcept {
	if (this->_p) {
		this->_p->use_count.fetch_add(1, std::memory_order_relaxed);
	}
}

void util::buffer::release() noexcept {
	if (this->_p && this->_p->use_count.fetch_sub(1, std::memory_order_release) == 1) {
		std::atomic_thread_fence(std::memory_order_acquire);

		void *base = this->_p->base;
		free(base);

		if (this->_p != base) {
			delete this->_p;
		}
	}

	this->reset();
}
