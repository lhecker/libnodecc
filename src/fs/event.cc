#include "libnodecc/fs/event.h"


namespace node {
namespace fs {

event::event() : uv::handle<uv_fs_event_t>() {
}

bool event::init(node::loop& loop) {
	return 0 == uv_fs_event_init(loop, *this);
}

bool event::start(const std::string& path) {
	return 0 == uv_fs_event_start(*this, [](uv_fs_event_t* handle, const char* filename, int events, int status) {
		auto self = reinterpret_cast<event*>(handle->data);

		if (self->on_event) {
			self->on_event(status, events, std::string(filename));
		}
	}, path.c_str(), 0);
}

bool event::stop() {
	return 0 == uv_fs_event_stop(*this);
}

} } // namespace node::fs
