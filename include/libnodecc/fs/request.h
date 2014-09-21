#ifndef nodecc_fs_h
#define nodecc_fs_h

#include <string>

#include "../uv/handle.h"


namespace node {
namespace fs {

class request : public node::uv::handle<uv_fs_t> {
	NODE_ADD_CALLBACK(finished, fs::request& req)

public:
	~request();

	bool init(node::loop& loop);

	bool open(const char* path, int flags, int mode, on_finished_t cb);

	bool read(uv_file file, const uv_buf_t bufs[], size_t nbufs, int64_t offset, on_finished_t cb);
	bool write(uv_file file, const uv_buf_t bufs[], size_t nbufs, int64_t offset, on_finished_t cb);

	bool mkdir(const char* path, int mode, on_finished_t cb);
	bool mkdtemp(const char* tpl, on_finished_t cb);
	bool rmdir(const char* path, on_finished_t cb);

	bool stat(const char* path, on_finished_t cb);
	bool fstat(uv_file file, on_finished_t cb);
	bool lstat(const char* path, on_finished_t cb);

	bool rename(const char* path, const char* new_path, on_finished_t cb);

	bool chmod(const char* path, int mode, on_finished_t cb);
	bool fchmod(uv_file file, int mode, on_finished_t cb);

	bool chown(const char* path, uv_uid_t uid, uv_gid_t gid, on_finished_t cb);
	bool fchown(uv_file file, uv_uid_t uid, uv_gid_t gid, on_finished_t cb);

	bool utime(const char* path, double atime, double mtime, on_finished_t cb);
	bool futime(uv_file file, double atime, double mtime, on_finished_t cb);

	bool readlink(const char* path, on_finished_t cb);

	bool fsync(uv_file file, on_finished_t cb);
	bool fdatasync(uv_file file, on_finished_t cb);

	bool ftruncate(uv_file file, int64_t offset, on_finished_t cb);

	bool sendfile(uv_file out_fd, uv_file in_fd, int64_t in_offset, size_t length, on_finished_t cb);

private:
	node::loop* loop;
};

} // namespace fs
} // namespace node

#endif // nodecc_fs_event_h
