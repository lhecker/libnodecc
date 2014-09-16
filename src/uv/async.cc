#include "libnodecc/uv/async.h"


uv::async::async() noexcept : uv::handle<uv_async_t>() {
	static_cast<uv_async_t*>(*this)->loop = nullptr;
}

bool uv::async::init(uv::loop& loop, uv_async_cb cb) {
	return 0 == uv_async_init(loop, *this, cb);
}

bool uv::async::send() {
	if (static_cast<uv_async_t*>(*this)->loop) {
		return 0 == uv_async_send(*this);
	} else {
		return false;
	}
}
