#include "libnodecc/uv/async.h"


namespace node {
namespace uv {

async::async(node::loop& loop, uv_async_cb cb) : handle<uv_async_t>() {
	node::uv::check(uv_async_init(loop, *this, cb));
}

void async::send() {
	node::uv::check(uv_async_send(*this));
}

} // namespace node
} // namespace uv
