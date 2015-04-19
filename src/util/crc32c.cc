#include "libnodecc/util/crc32c.h"

#include "crc32c_8x256_tables.cc"


namespace node {
namespace util {

uint32_t crc32c::checksum(node::buffer_view buffer) {
	return crc32c::push(0xffffffff, buffer) ^ 0xffffffff;
}

crc32c::crc32c() : _crc(0xffffffff) {
}

void crc32c::push(node::buffer_view buffer) {
	this->_crc = crc32c::push(this->_crc, buffer);
}

uint32_t crc32c::checksum() const {
	return this->_crc ^ 0xffffffff;
}

uint32_t crc32c::push(uint32_t crc, node::buffer_view buffer) {
	// implements CRC32C divide-by-8, similiar to one of Intel's implementations
	const uint8_t* data = buffer.data<const uint8_t>();
	const uint8_t* dataend = data + buffer.size();

	if (!data || data == dataend) {
		return 0xffffffff;
	}

	const uint8_t* div8start = (const uint8_t*)((uintptr_t(data) + 7) & ~7);
	const uint8_t* div8end = (const uint8_t* const)(uintptr_t(dataend) & ~7);

	uint32_t term1;
	uint32_t term2;

	if (div8start > dataend) {
		div8start = dataend;
	}

	while (data < div8start) {
		crc = crc_tableil8_o32[(crc ^ *data++) & 0x000000ff] ^ (crc >> 8);
	}

	while (data < div8end) {
		crc ^= *(const uint32_t*)data;

		data += 4;

		term1 = crc_tableil8_o88[crc & 0x000000ff] ^ crc_tableil8_o80[(crc >> 8) & 0x000000ff];
		term2 = crc >> 16;

		crc = term1 ^ crc_tableil8_o72[term2 & 0x000000ff] ^ crc_tableil8_o64[(term2 >> 8) & 0x000000ff];

		term1 = crc_tableil8_o56[(*(const uint32_t*)data) & 0x000000ff] ^ crc_tableil8_o48[((*(uint32_t*)data) >> 8) & 0x000000ff];
		term2 = (*(const uint32_t*)data) >> 16;

		data += 4;

		crc = crc ^ term1 ^ crc_tableil8_o40[term2 & 0x000000ff] ^ crc_tableil8_o32[(term2 >> 8) & 0x000000ff];
	}

	while (data < dataend) {
		crc = crc_tableil8_o32[(crc ^ *data++) & 0x000000ff] ^ (crc >> 8);
	}

	return crc;
}

} // namespace util
} // namespace node
