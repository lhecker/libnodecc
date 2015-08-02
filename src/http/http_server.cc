#include "libnodecc/http/server.h"

#include "libnodecc/util/base64.h"
#include "libnodecc/util/sha1.h"


namespace node {
namespace http {

server::server() : net::server() {
	this->on_connection([this]() {
		const auto socket = std::make_shared<net::socket>();
		const auto req = std::make_shared<incoming_message>(socket, HTTP_REQUEST);
		const auto res = std::make_shared<server_response>(socket);

		if (!this->accept(*socket.get())) {
			return;
		}

		socket->on_close([req]() {
			req->_close();
		});

		// callback (and thus the reference to pack) will be released if an error in socket.on_read() is returned (e.g. EOF)
		req->on_headers_complete([this, req, res](bool upgrade, bool keep_alive) {
			using namespace node::literals;

			res->_shutdown_on_end = !keep_alive;

			// RFC 2616 - 14.23
			if (!req->has_header("host"_view)) {
				res->set_status_code(400);
				res->end();
				return;
			}

			if (!this->on_request.emit(req, res)) {
				res->set_status_code(500);
				res->end();
				return;
			}

			if (upgrade) {
				if (req->_is_websocket == 1) {
					static const auto websocketMagic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_view;
					const auto key = req->header("sec-websocket-key"_view);
					uint8_t digest[SHA1_DIGEST_LENGTH];

					node::mutable_buffer buffer;
					buffer.set_capacity(key.size() + websocketMagic.size());
					buffer.append(key);
					buffer.append(websocketMagic);

					node::util::sha1 s;
					s.push(buffer);
					s.get_digest(digest);

					const auto acceptHash = node::util::base64::encode(node::buffer_view(digest, sizeof(digest)));

					res->_shutdown_on_end = true;
					res->set_status_code(101);
					res->set_header("connection"_view,           "upgrade"_view);
					res->set_header("upgrade"_view,              "websocket"_view);
					res->set_header("sec-websocket-accept"_view, acceptHash);
					res->send_headers();
					return;
				}

				res->set_status_code(501);
				res->end();
			}
		});

		socket->resume();
	});
}

} // namespace node
} // namespace http
