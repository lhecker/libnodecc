#ifndef nodecc_util_crc32c_h
#define nodecc_util_crc32c_h

#include "../buffer.h"


namespace node {
namespace util {

class crc32c {
public:
	static uint32_t checksum(node::buffer_view buffer);

	explicit crc32c();

	void push(node::buffer_view buffer);
	uint32_t checksum() const;

private:
	static uint32_t push(uint32_t crc, node::buffer_view buffer);

	uint32_t _crc;
};

} // namespace util
} // namespace node

#endif // nodecc_util_crc32c_h
