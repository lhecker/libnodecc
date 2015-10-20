#ifndef nodecc_uv_async_h
#define nodecc_uv_async_h

#include "handle.h"


namespace node {
class loop;
}


namespace node {
namespace uv {

class async : public node::uv::handle<uv_async_t> {
public:
	explicit async(node::loop& loop, uv_async_cb cb);

	void send();

protected:
	~async() override = default;
};

} // namespace uv
} // namespace node

#endif // nodecc_uv_async_h
