#ifndef nodecc_fs_event_h
#define nodecc_fs_event_h

#include <string>

#include "../uv/handle.h"


namespace node {
namespace fs {

class event : public node::uv::handle<uv_fs_event_t> {
public:
	NODE_CALLBACK_ADD(public, event, void, int err, int events, const std::string& str);

public:
	explicit event();

	bool init(node::loop& loop);

	bool start(const std::string& path);
	bool stop();
};

} // namespace fs
} // namespace node

#endif // nodecc_fs_event_h
