#ifndef nodecc_fs_event_h
#define nodecc_fs_event_h

#include <string>

#include "../uv/handle.h"


namespace fs {

class event : public uv::handle<uv_fs_event_t> {
public:
	typedef std::function<void(int err, int events, const std::string& str)> on_event_t;


	explicit event();

	bool init(uv::loop& loop);

	bool start(const std::string& path);
	bool stop();


	on_event_t on_event;
};

} // namespace fs

#endif // nodecc_fs_event_h
