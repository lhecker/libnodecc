#include "libnodecc/buffer.h"

#include <algorithm>


namespace node {

buffer_ref_list::buffer_ref_list(const node::buffer bufs[], size_t bufcnt) noexcept : buffer_ref_list() {
	this->resize(bufcnt);

	for (size_t i = 0; i < bufcnt; i++) {
		this->push_front(bufs[i]);
	}
}

buffer_ref_list::~buffer_ref_list() noexcept {
	this->clear();
}

void buffer_ref_list::push_front(const buffer& ref) noexcept {
	const_cast<buffer&>(ref)._retain();
	this->_push_front(ref);
}

void buffer_ref_list::emplace_front(buffer&& ref) noexcept {
	this->_push_front(ref);
	ref._reset_unreleased();
}

void buffer_ref_list::resize(size_t size) noexcept {
	if (size > this->_capacity) {
		const size_t capacity = std::max({ size, this->_capacity + (this->_capacity >> 1) });
		const auto list = new(std::nothrow) buffer::control_base*[capacity];

		if (list) {
			if (this->_size == 1 && this->_capacity == 1) {
				list[0] = static_cast<buffer::control_base*>(this->_list);
			}

			this->_list = list;
			this->_capacity = capacity;
		}
	}
}

void buffer_ref_list::clear() noexcept {
	if (this->_size == 1) {
		static_cast<buffer::control_base*>(this->_list)->release();
	} else {
		for (size_t i = 0; i < this->_size; i++) {
			static_cast<buffer::control_base**>(this->_list)[i]->release();
		}
	}
}

void buffer_ref_list::swap(buffer_ref_list& other) noexcept {
	std::swap(this->_list, other._list);
	std::swap(this->_size, other._size);
}

void buffer_ref_list::_push_front(const buffer& ref) noexcept {
	const auto p = ref._p;

	if (p) {
		const auto current_size = this->_size;
		const auto new_size = current_size + 1;

		if (current_size == 0) {
			this->_list = p;
		} else {
			this->resize(new_size);
			static_cast<buffer::control_base**>(this->_list)[current_size] = p;
		}

		this->_size = new_size;
	}
}

} // namespace node
