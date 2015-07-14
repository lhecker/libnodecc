#include "libnodecc/util/uri.h"

#include <stdexcept>


static uint8_t hex_to_int(uint8_t c) {
	const uint8_t lo = c & 0x0F;
	const uint8_t hi = c & 0xF0;

	switch (hi) {
	case 0x30:
		if (lo <= 9) {
			return lo;
		}
	case 0x40:
	case 0x60:
		if (lo >= 1 && lo <= 6) {
			return 9 + lo;
		}
	}

	throw std::range_error("invalid hex char");
}


namespace node {
namespace util {

node::buffer uri::component_decode(const buffer_view buffer, bool urlencoded) {
	try {
		// TODO: UTF-8 support
		// TODO: use result.set_capacity() to minimize allocations
		node::mutable_buffer result;

		const uint8_t* base = buffer.data();
		const size_t size = buffer.size();

		for (size_t i = 0; i < size; i++) {
			uint8_t c = base[i];

			if (c < 0x20 || c >= 0x7F) {
				throw std::range_error("non ASCII char");
			} else if (urlencoded && c == '+') {
				c = ' ';
			} else if (c == '%' && (size - i) > 2) {
				const uint8_t lo = hex_to_int(base[i + 1]);
				const uint8_t hi = hex_to_int(base[i + 2]);

				c = (hi << 4) + lo;
				i += 2;
			}

			result.push_back(c);
		}

		return result;
	} catch (...) {
		return node::buffer();
	}
}

} // namespace util
} // namespace node
