#ifndef nodecc_util_uri_h
#define nodecc_util_uri_h

#include "../buffer.h"


namespace node {
namespace util {

class uri {
public:
	static node::buffer component_decode(const node::buffer_view buffer, bool urlencoded = false);
};

} // namespace util
} // namespace node


#endif // nodecc_util_uri_h
