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


util::buffer::buffer(const void *base, size_t size, util::flags flags) noexcept : _p(nullptr), _data((void*)base), _size(size) {
	switch (flags) {
		case util::flags::strong:
			this->_p = new control((void*)base);
		case util::flags::copy:
			this->copy();
		default:
			;
	}
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

util::buffer::buffer(size_t size) noexcept {
	if (size > 0) {
		uint8_t *base = (uint8_t*)malloc(sizeof(control) + size);
		uint8_t *data = base + sizeof(control);

		if (base) {
			this->_p = new(base) control(base);
			this->_data = data;
			this->_size = size;
			return;
		}
	}

	util::buffer::buffer();
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
	util::buffer::buffer(base, size, flags);
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

util::buffer util::buffer::copy(size_t size) const noexcept {
	util::buffer buffer;

	if (size == 0) {
		size = this->_size;
	}

	uint8_t *base = (uint8_t*)malloc(sizeof(control) + size);
	uint8_t *data = base + sizeof(control);

	if (base) {
		if (this->_data) {
			memcpy(data, this->_data, std::min(size, this->_size));
		}

		buffer._p = new(base) control(base);
		buffer._data = data;
		buffer._size = size;
	}

	return buffer;
}

util::buffer util::buffer::slice(ssize_t start, ssize_t end) const noexcept {
	util::buffer buffer;

	if (this->_size < size_t(SSIZE_MAX) && this->_data) {
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

		if (end < start) {
			end = start;
		}

		buffer._p = this->_p;
		buffer._data = this->get() + start;
		buffer._size = end - start;
		buffer.retain();
	}

	return buffer;
}

void util::buffer::swap(util::buffer &other) noexcept {
	std::swap(this->_p, other._p);
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
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
