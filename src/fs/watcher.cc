#include "libnodecc/fs/watcher.h"


namespace node {
namespace fs {

decltype(watcher::event) watcher::event;


watcher::watcher(node::loop& loop) : uv::handle<uv_fs_event_t>() {
	uv_fs_event_init(loop, *this);
}

bool watcher::start(const std::string& path) {
	return 0 == uv_fs_event_start(*this, [](uv_fs_event_t* handle, const char* filename, int events, int status) {
		auto self = reinterpret_cast<watcher*>(handle->data);

		if (status == 0) {
			self->emit(event, events, std::string(filename));
		} else {
			self->emit(error_event, node::uv::to_error(status));
		}
	}, path.c_str(), 0);
}

bool watcher::stop() {
	return 0 == uv_fs_event_stop(*this);
}

} // namespace fs
} // namespace node
