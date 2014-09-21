#include "libnodecc/uv/async.h"


namespace node {
namespace uv {

async::async() noexcept : handle<uv_async_t>() {
	static_cast<uv_async_t*>(*this)->loop = nullptr;
}

bool async::init(node::loop& loop, uv_async_cb cb) {
	return 0 == uv_async_init(loop, *this, cb);
}

bool async::send() {
	if (static_cast<uv_async_t*>(*this)->loop) {
		return 0 == uv_async_send(*this);
	} else {
		return false;
	}
}

} // namespace node
} // namespace uv
