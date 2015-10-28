#ifndef nodecc_uv_async_h
#define nodecc_uv_async_h

#include "handle.h"


namespace node {
namespace uv {

class async : public node::uv::handle<uv_async_t> {
public:
	static const node::events::type<void()> async_event;

	explicit async(node::loop& loop);
	void send();

protected:
	~async() override = default;
};

} // namespace uv
} // namespace node

#endif // nodecc_uv_async_h
