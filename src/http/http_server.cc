#include "libnodecc/http/server.h"

#include <uv.h>
#include <string>


namespace node {
namespace http {

server::server() : net::server() {
	this->clients.max_load_factor(0.75);

	this->on_connection([this]() {
		auto iter = this->clients.emplace().first;
		net::socket& socket = const_cast<net::socket&>(*iter);

		if (!this->accept(socket)) {
			this->clients.erase(iter);
		}

		incoming_message* req = new incoming_message(socket, HTTP_REQUEST);
		server_response*  res = new server_response(socket);

		socket.on_close([this, req, res]() {
			req->_close();

			const net::socket& socket = req->socket;

			delete req;
			delete res;
			this->clients.erase(socket);
		});

		req->on_headers_complete([this, req, res](bool keep_alive) {
			res->_shutdown_on_end = keep_alive;

			if (!this->emit_request_s(*req, *res)) {
				res->socket().close();
			}
		});

		socket.unref();
		socket.read_start();
	});
}

} // namespace node
} // namespace http
