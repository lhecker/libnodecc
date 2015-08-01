#include "libnodecc/http/server.h"

#include "libnodecc/util/base64.h"
#include "libnodecc/util/sha1.h"


namespace node {
namespace http {

class server::req_res_pack {
public:
	explicit req_res_pack(server* server) : req(socket, HTTP_REQUEST), res(socket), server(server) {
	}

	std::weak_ptr<req_res_pack> next;
	node::net::socket socket;
	node::http::incoming_message req;
	node::http::server_response res;
	server* server;
};

server::server() : net::server() {
	this->on_connection([this]() {
		auto pack = std::make_shared<req_res_pack>(this);

		if (!this->accept(pack->socket)) {
			return;
		}

		// enqueue pack at the head of the single-linked list of clients
		{
			auto currentHead = this->_clients_head.lock();

			if (currentHead) {
				currentHead->next = pack;
			}
		}

		this->_clients_head = pack;

		pack->socket.on_close([pack]() {
			pack->req._close();
		});

		// callback (and thus the reference to pack) will be released if an error in socket.on_read() is returned (e.g. EOF)
		pack->req.on_headers_complete([pack](bool upgrade, bool keep_alive) {
			using namespace node::literals;

			pack->res._shutdown_on_end = !keep_alive;

			// RFC 2616 - 14.23
			if (!pack->req.has_header("host"_hashed_view)) {
				pack->res.set_status_code(400);
				pack->res.end();
				return;
			}

			if (!pack->server || !pack->server->on_request.emit(request(pack, &pack->req), response(pack, &pack->res))) {
				pack->res.set_status_code(500);
				pack->res.end();
				return;
			}

			if (upgrade) {
				if (pack->req._is_websocket == 1) {
					static const auto websocketMagic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_hashed_view;
					const auto key = pack->req.header("sec-websocket-key"_hashed_view);
					uint8_t digest[SHA1_DIGEST_LENGTH];

					node::mutable_buffer buffer;
					buffer.set_capacity(key.size() + websocketMagic.size());
					buffer.append(key);
					buffer.append(websocketMagic);

					node::util::sha1 s;
					s.push(buffer);
					s.get_digest(digest);

					const auto acceptHash = node::util::base64::encode(node::hashed_view(digest, sizeof(digest)));

					pack->res._shutdown_on_end = true;
					pack->res.set_status_code(101);
					pack->res.set_header("connection"_hashed_view,           "upgrade"_hashed_view);
					pack->res.set_header("upgrade"_hashed_view,              "websocket"_hashed_view);
					pack->res.set_header("sec-websocket-accept"_hashed_view, acceptHash);
					pack->res.send_headers();
					return;
				}

				pack->res.set_status_code(501);
				pack->res.end();
			}
		});

		pack->socket.resume();
	});
}

void server::close() {
	auto it = this->_clients_head;

	while (true) {
		auto pack = it.lock();

		if (pack) {
			pack->server = nullptr;
			pack->socket.close();
			it = pack->next;
		} else {
			break;
		}
	}

	node::net::server::close();
}

} // namespace node
} // namespace http
