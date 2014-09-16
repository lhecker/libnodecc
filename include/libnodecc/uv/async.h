#ifndef nodecc_uv_async_h
#define nodecc_uv_async_h

#include "handle.h"
#include "loop.h"


namespace uv {

class async : public uv::handle<uv_async_t> {
public:
	explicit async() noexcept;

	bool init(uv::loop& loop, uv_async_cb cb);
	bool send();
};

} // namespace uv

#endif // nodecc_uv_async_h
