#include "libnodecc/fs.h"


namespace node {
namespace fs {

request::~request() {
	uv_fs_req_cleanup(*this);
}

} // namespace node
} // namespace fs
