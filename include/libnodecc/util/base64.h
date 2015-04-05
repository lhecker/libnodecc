#ifndef nodecc_util_base64_h
#define nodecc_util_base64_h

#include "../buffer.h"


namespace node {
namespace util {

class base64 {
public:
	/**
	 * Encodes a buffer using base64 or base64url.
	 */
	static node::buffer encode(const node::buffer_view buffer, bool base64url = false);

	/**
	 * Decodes a base64 encoded buffer.
	 *
	 * WARNING: This decoder simply ignores invalid characters.
	 */
	static node::buffer decode(const node::buffer_view buffer);
};

} // namespace util
} // namespace node


#endif // nodecc_util_base64_h
