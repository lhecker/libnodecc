#ifndef nodecc_fs_watcher_h
#define nodecc_fs_watcher_h

#include <string>

#include "../uv/handle.h"


namespace node {
namespace fs {

class watcher : public node::uv::handle<uv_fs_event_t> {
public:
	static const node::events::symbol<void(int events, const std::string& str)> event;

	explicit watcher(node::loop& loop);

	bool start(const std::string& path);
	bool stop();
};

} // namespace fs
} // namespace node

#endif // nodecc_fs_watcher_h
