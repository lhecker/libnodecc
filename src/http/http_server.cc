#include "libnodecc/http/server.h"

#include <uv.h>
#include <string>


http::server::server() : net::server() {
	this->clients.max_load_factor(0.75);

	this->on_connection = [this]() {
		auto iter = this->clients.emplace().first;
		net::socket &socket = const_cast<net::socket&>(*iter);

		if (!socket.init(*this) || !this->accept(socket)) {
			this->clients.erase(iter);
		}

		http::incoming_message *req = new http::incoming_message(socket);
		http::server_response  *res = new http::server_response(socket);

		socket.on_close = [this, req, res]() {
			const net::socket &socket = req->socket;
			delete req;
			delete res;
			this->clients.erase(socket);
		};

		req->on_headers_complete = [this, req, res]() {
			const auto iter = req->headers.find("connection");
			bool hasConnectionHeader = iter != req->headers.end();
			bool isHTTP11 = req->http_version_major == 1 && req->http_version_major == 1;

			if (isHTTP11) {
				if (!hasConnectionHeader || iter->second != "keep-alive") {
					res->_shutdown_on_end = true;
				}
			} else {
				if (hasConnectionHeader && iter->second == "close") {
					res->_shutdown_on_end = true;
				}
			}

			if (this->on_request) {
				this->on_request(*req, *res);
			} else {
				res->socket.shutdown();
			}
		};

		socket.unref();
		socket.read_start();
	};
}
