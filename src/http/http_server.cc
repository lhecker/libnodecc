#include "libnodecc/http/server.h"

#include <uv.h>


namespace node {
namespace http {

class server::req_res_pack {
public:
	explicit req_res_pack() : req(socket, HTTP_REQUEST), res(socket) {
	}

	req_res_pack* prev;
	req_res_pack* next;
	node::net::socket socket;
	node::http::incoming_message req;
	node::http::server_response res;
};

server::server() : net::server(), _clients_head(nullptr) {
	this->on_connection([this]() {
		req_res_pack* pack = new req_res_pack();

		if (!this->accept(pack->socket)) {
			delete pack;
		}

		// enqueue pack at the head of the double-linked list of clients
		{
			if (this->_clients_head) {
				this->_clients_head->prev = pack;
			}

			pack->prev = nullptr;
			pack->next = this->_clients_head;

			this->_clients_head = pack;
		}

		pack->socket.on_close([this, pack]() {
			if (pack->prev) {
				pack->prev->next = pack->next;
			} else {
				// pack has no previous element ---> it must be head
				this->_clients_head = pack->next;
			}

			if (pack->next) {
				pack->next->prev = pack->prev;
			}

			delete pack;
		});

		pack->req.on_headers_complete([this, pack](bool keep_alive) {
			pack->res._shutdown_on_end = keep_alive;

			// RFC 2616 - 14.23
			if (!pack->req.headers.count("host")) {
				pack->res.set_status_code(400);
				pack->res.end();
				return;
			}

			if (!this->emit_request_s(pack->req, pack->res)) {
				pack->res.set_status_code(500);
				pack->res.end();
			}
		});

		pack->socket.read_start();
	});
}

void server::close() {
	req_res_pack* pack = this->_clients_head;

	while (pack) {
		pack->socket.close();
		pack = pack->next;
	}

	node::net::server::close();
}

} // namespace node
} // namespace http
