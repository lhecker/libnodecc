#include "libnodecc/fs.h"


std::shared<node::stream::readable<node::buffer, int>> create_read_stream(const char* path) {
	return std::make_shared<readable_stream>(path);
}
