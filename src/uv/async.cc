#include "libnodecc/uv/async.h"


namespace node {
namespace uv {

decltype(async::async_event) async::async_event;


async::async(node::loop& loop) : handle<uv_async_t>() {
	node::uv::check(uv_async_init(loop, *this, [](uv_async_t* handle) {
		auto self = reinterpret_cast<async*>(handle->data);

		self->emit(async_event);
	}));
}

void async::send() {
	node::uv::check(uv_async_send(*this));
}

} // namespace node
} // namespace uv
