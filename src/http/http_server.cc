#include "libnodecc/http/server.h"

#include <openssl/sha.h>
#include <wslay/wslay.h>

#include "libnodecc/util/base64.h"


namespace node {
namespace http {

class server::req_res_pack {
public:
	explicit req_res_pack(server* server) : server(server), req(socket, HTTP_REQUEST), res(socket) {
	}

	server* server;
	req_res_pack* prev;
	req_res_pack* next;
	node::net::socket socket;
	node::http::incoming_message req;
	node::http::server_response res;
};

server::server() : net::server(), _clients_head(nullptr) {
	this->on_connection([this]() {
		req_res_pack* pack = new req_res_pack(this);

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

		pack->socket.on_close([pack]() {
			if (pack->prev) {
				pack->prev->next = pack->next;
			} else if (pack->server) {
				// pack has no previous element ---> it must be head
				pack->server->_clients_head = pack->next;
			}

			if (pack->next) {
				pack->next->prev = pack->prev;
			}

			delete pack;
		});

		pack->req.on_headers_complete([pack](bool upgrade, bool keep_alive) {
			pack->res._shutdown_on_end = !keep_alive;

			// RFC 2616 - 14.23
			if (!pack->req.headers().count("host")) {
				pack->res.set_status_code(400);
				pack->res.end();
				return;
			}

			if (!pack->server || !pack->server->emit_request_s(pack->req, pack->res)) {
				pack->res.set_status_code(500);
				pack->res.end();
				return;
			}

			if (upgrade) {
				if (pack->req._is_websocket == 1) {
					uint8_t digest[SHA_DIGEST_LENGTH];
					const char websocketMagic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
					const auto& key = pack->req._headers.at("sec-websocket-key");

					node::mutable_buffer buffer;
					buffer.reserve(key.length() + strlen(websocketMagic));
					buffer.append(key);
					buffer.append(websocketMagic);

					if (SHA1(buffer.data<const uint8_t>(), buffer.size(), digest)) {
						pack->res._shutdown_on_end = true;
						pack->res.set_status_code(101);
						pack->res.set_header("connection", "upgrade");
						pack->res.set_header("upgrade", "websocket");
						pack->res.set_header("sec-websocket-accept", node::util::base64::encode(node::buffer(digest, node::weak)).string());
						pack->res.send_headers();
						return;
					}
				}

				pack->res.set_status_code(501);
				pack->res.end();
			}
		});

		pack->socket.read_start();
	});
}

void server::close() {
	req_res_pack* pack = this->_clients_head;

	while (pack) {
		pack->server = nullptr;
		pack->socket.close();
		pack = pack->next;
	}

	node::net::server::close();
}

} // namespace node
} // namespace http
