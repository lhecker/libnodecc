#include "libnodecc/fs/event.h"


namespace node {
namespace fs {

event::event(node::loop& loop) : uv::handle<uv_fs_event_t>() {
	uv_fs_event_init(loop, *this);
}

bool event::start(const std::string& path) {
	return 0 == uv_fs_event_start(*this, [](uv_fs_event_t* handle, const char* filename, int events, int status) {
		auto self = reinterpret_cast<event*>(handle->data);

		self->event_callback.emit(status, events, std::string(filename));

		if (status != 0) {
			self->event_callback.connect(nullptr);
		}
	}, path.c_str(), 0);
}

bool event::stop() {
	return 0 == uv_fs_event_stop(*this);
}

} // namespace fs
} // namespace node
