#include "libnodecc/util/buffer.h"

#include <atomic>
#include <cstdlib>

#include "libnodecc/util/string.h"


namespace util {

struct buffer::control {
	constexpr control(void* base) noexcept : base(base), use_count(1) {};

	void* base;
	std::atomic<unsigned int> use_count;
};

}


util::buffer::buffer(const void* base, size_t size, util::flags flags) noexcept : buffer() {
	this->reset(base, size, flags);
}

util::buffer::buffer(const buffer& other) noexcept : _p(other._p), _data(other._data), _size(other._size) {
	this->retain();
}

util::buffer& util::buffer::operator=(const buffer& other) noexcept {
	this->release();
	this->_p = other._p;
	this->_data = other._data;
	this->_size = other._size;
	this->retain();

	return *this;
}

util::buffer::buffer(size_t size) noexcept {
	this->reset(size);
}

util::buffer::~buffer() noexcept {
	this->release();
}

void util::buffer::swap(util::buffer& other) noexcept {
	std::swap(this->_p, other._p);
	std::swap(this->_data, other._data);
	std::swap(this->_size, other._size);
}

void util::buffer::reset() noexcept {
	this->release();
}

void util::buffer::reset(size_t size) noexcept {
	if (size > 0) {
		uint8_t* base = (uint8_t*)malloc(sizeof(control) + size);
		uint8_t* data = base + sizeof(control);

		if (base) {
			this->_p = new(base) control(base);
			this->_data = data;
			this->_size = size;
			return;
		}
	}

	this->_p = nullptr;
	this->_data = nullptr;
	this->_size = 0;
}

void util::buffer::reset(const void* base, size_t size, util::flags flags) noexcept {
	this->release();

	this->_data = (void*)base;
	this->_size = size;

	switch (flags) {
	case util::flags::strong:
		this->_p = new control((void*)base);
		break;
	case util::flags::copy:
		this->copy(*this);
		break;
	default:
		;
	}
}

util::buffer util::buffer::copy(size_t size) const noexcept {
	util::buffer buffer;
	this->copy(buffer, size);
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

bool util::buffer::is_strong() const noexcept {
	return this->_p;
}

bool util::buffer::is_weak() const noexcept {
	return !this->_p;
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

char& util::buffer::operator[](size_t pos) const noexcept {
	return *(this->data<char>() + pos);
}

util::buffer::operator bool() const noexcept {
	return this->_p != nullptr;
}

size_t util::buffer::use_count() const noexcept {
	return this->_p ? this->_p->use_count.load(std::memory_order_relaxed) : 0;
}

uint8_t* util::buffer::get() const noexcept {
	return this->data<uint8_t>();
}

size_t util::buffer::size() const noexcept {
	return this->_size;
}

void util::buffer::copy(util::buffer& target, std::size_t size) const noexcept {
	if (size == 0) {
		size = this->_size;
	}

	uint8_t* base = (uint8_t*)malloc(sizeof(control) + size);
	uint8_t* data = base + sizeof(control);

	if (base) {
		if (this->_data) {
			memcpy(data, this->_data, std::min(size, this->_size));
		}

		target._p = new(base) control(base);
		target._data = data;
		target._size = size;
	} else {
		target._p = nullptr;
		target._data = nullptr;
		target._size = 0;
	}
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

bool operator==(const util::buffer& lhs, const util::buffer& rhs) noexcept {
	return lhs.data() == rhs.data();
}

bool operator!=(const util::buffer& lhs, const util::buffer& rhs) noexcept {
	return lhs.data() != rhs.data();
}
