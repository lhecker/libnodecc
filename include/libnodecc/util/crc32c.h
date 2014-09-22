#ifndef nodecc_util_crc32c_h
#define nodecc_util_crc32c_h

#include "../buffer.h"


namespace node {
namespace util {

class crc32c {
public:
	explicit crc32c();

	void push(const void* data, size_t size);
	void push(const node::buffer& buffer);

	uint32_t checksum() const;

	static uint32_t checksum(const void* data, size_t size);
	static uint32_t checksum(const node::buffer& buffer);

private:
	static uint32_t push(uint32_t crc, const void* data, size_t size);

	uint32_t _crc;
};

} // namespace util
} // namespace node

#endif // nodecc_util_crc32c_h
