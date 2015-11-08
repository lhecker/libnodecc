libnodecc
=========

This project is still heavily WIP.

## Build Instructions

Requirements:
- C++11 Compiler (Xcode 6, Visual Studio 2013, gcc, …)
- Python 2.6 or newer

The general build process is similiar to building [libuv](https://github.com/joyent/libuv).

### OS X / Linux

    $ mkdir -p build && git clone https://chromium.googlesource.com/external/gyp build/gyp
    $ ./configure -f [cmake|eclipse|make|msvs|ninja|xcode]

### Windows

Almost everything should work right away but some bits and pieces haven't been ported yet due to lack of time.

### Example

The following code will create a simple TCP IPv4/6 echo server:

```cpp
#include <libnodecc/tcp/server.h>

int main() {
	node::loop loop;

	auto server = node::make_shared<node::tcp::server>(loop);
	server->listen6(8080);

	server->on(server->connection_event, [server]() {
		auto client = node::make_shared<node::tcp::socket>(server->loop());
		server->accept(*client);

		client->on(client->data_event, [client](const node::buffer& buffer) {
			client->write(buffer);
		});

		client->resume();
	});

	loop.run();

	return 0;
}
```

The following code will create a simple HTTP server, which replies with all request headers:

```cpp
#include <libnodecc/http/server.h>

int main() {
	node::loop loop;

	auto server = node::make_shared<node::http::server>(loop);
	server->listen6(8080);

	server->on(server->request_event, [](const node::http::server::request& req, const node::http::server::response& res) {
		using namespace node::literals;

		node::mutable_buffer buf;

		for (const auto& it : req->headers()) {
			buf.append(it.first);
			buf.append(": ");
			buf.append(it.second);
			buf.push_back('\n');
		}

		res->set_header("content-type"_view, "text/plain"_view);
		res->end(buf);
	});

	loop.run();

	return 0;
}
```
