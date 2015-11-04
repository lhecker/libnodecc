#include "libnodecc/buffer.h"

#include <algorithm>


namespace node {

const std::size_t buffer_view::npos;


buffer_view& buffer_view::operator=(const buffer_view& other) {
	this->_data = other._data;
	this->_size = other._size;
	return *this;
}

buffer_view buffer_view::view(std::size_t beg, std::size_t end) const noexcept {
	const std::size_t cbeg = std::min(beg, this->size());
	const std::size_t cend = std::min(end, this->size());
	return buffer_view(this->data() ? this->data() + cbeg : nullptr, cend > cbeg ? cend - cbeg : 0);
}

bool buffer_view::equals(const buffer_view& other) const noexcept {
	return this->size() == other.size()
	    && (
	           this->data() == other.data()
	        || (
	               this->data() != nullptr
	            && other.data() != nullptr
	            && memcmp(this->data(), other.data(), this->size()) == 0
	        )
	    );
}

std::size_t buffer_view::index_of(const char ch) const noexcept {
	if (this->data()) {
		const uint8_t* pos = static_cast<uint8_t*>(memchr(this->data(), ch, this->size()));

		if (pos) {
			return static_cast<std::size_t>(pos - this->data());
		}
	}

	return npos;
}

std::size_t buffer_view::index_of(const buffer_view& other) const noexcept {
	const uint8_t* data1     = this->data();
	const uint8_t* data1_end = data1 + this->size();
	const uint8_t* data2     = other.data();
	const uint8_t* data2_end = data2 + other.size();

	if (data1 && data2) {
		const uint8_t* pos = std::search(data1, data1_end, data2, data2_end);
		return pos == data1_end ? npos : static_cast<std::size_t>(pos - data1);
	} else {
		return npos;
	}
}

std::unique_ptr<char> buffer_view::create_c_str() const {
	std::unique_ptr<char> ret;

	if (this->size()) {
		ret.reset(new char[this->size() + 1]);
		memcpy(ret.get(), this->data(), this->size());
		ret.get()[this->size()] = '\0';
	}

	return ret;
}


bool operator==(node::buffer_view& lhs, node::buffer_view& rhs) noexcept {
	return lhs.data() == rhs.data() && lhs.size() == rhs.size();
}

bool operator!=(node::buffer_view& lhs, node::buffer_view& rhs) noexcept {
	return lhs.data() != rhs.data() || lhs.size() != rhs.size();
}

} // namespace node
