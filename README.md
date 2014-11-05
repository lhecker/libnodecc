libnodecc
=========

This project is still heavily WIP.

## Build Instructions

Requirements:
- C++11 Compiler (Xcode 6, Visual Studio 2013, gcc, …)
- Python 2.6 or newer
- [gyp](https://code.google.com/p/gyp/) in the ./build directory


To build this project GYP is required as a meta-build system.
The general build process is similia to building [libuv](https://github.com/joyent/libuv).

### OS X / Linux

    $ mkdir -p build && git clone https://git.chromium.org/external/gyp.git build/gyp
    $ ./configure -f [cmake|eclipse|make|msvs|ninja|xcode]

### Windows

Not yet tested, but try using ./configure directly with python 2.x.

### Example

```cpp
#include <libnodecc/net/server.h>

int main() {
	node::loop loop;

	node::net::server server;
	server.init(loop);
	server.listen6();

	server.on_connection([&server]() {
		auto client = std::make_shared<node::net::socket>();
		server.accept(*client);

		client->on_read([client](int err, const node::buffer& buffer) {
			if (!err) {
				client->write(buffer);
			}
		});

		client->read_start();
	});

	loop.run();

	return 0;
}
```
