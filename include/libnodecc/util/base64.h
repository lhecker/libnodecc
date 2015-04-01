#ifndef nodecc_util_base64_h
#define nodecc_util_base64_h

#include "../buffer.h"


namespace node {
namespace util {

class base64 {
public:
	static node::buffer encode(const node::buffer_view buffer);
	static node::buffer decode(const node::buffer_view buffer);
};

} // namespace util
} // namespace node


#endif // nodecc_util_base64_h
