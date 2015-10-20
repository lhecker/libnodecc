#include "libnodecc/http/server.h"

#include "_status_codes.h"
#include "libnodecc/http/_http_date_buffer.h"
#include "libnodecc/util/base64.h"
#include "libnodecc/util/sha1.h"


static inline uv_buf_t str_status_code(uint16_t status_code) {
	uv_buf_t ret;

	/*
	 * Most (good) compilers will layout this long case/switch
	 * in a nested one (e.g. first switching 1xx/2xx/3xx/4xx/5xx
	 * and then switching between the lower numbers).
	 *
	 * TODO: Manually optimize for better predictability.
	 */
	switch (status_code) {
#define XX(_status_, _len_, _str_) case _status_: ret.len = _len_; ret.base = (char*)_str_; break;
	STATUS_CODES(XX)
#undef XX

	default: ret.len = 0; ret.base = nullptr; break;
	}

	return ret;
}


static node::http::http_date_buffer date_buffer;


namespace node {
namespace http {

server::server_response::server_response(const node::shared_ptr<node::tcp::socket>& socket) : outgoing_message(socket), _status_code(200), _shutdown_on_end(true) {
}

uint16_t server::server_response::status_code() const {
	return this->_status_code;
}

void server::server_response::set_status_code(uint16_t code) {
	this->_status_code = code;
}

void server::server_response::compile_headers(node::mutable_buffer& buf) {
	{
		uv_buf_t status = str_status_code(this->status_code());

		if (!status.base) {
			status = str_status_code(500);
		}

		/*
		 * As per RFC 2145 §2.3:
		 *
		 * An HTTP server SHOULD send a response version equal to the highest
		 * version for which the server is at least conditionally compliant, and
		 * whose major version is less than or equal to the one received in the
		 * request.
		 */
		buf.append("HTTP/1.1 ");
		buf.append(status.base, status.len);
		buf.append("\r\n");
	}

	if (this->_headers.find("date"_view) == this->_headers.end()) {
		date_buffer.update(uv_now(*this->socket().get()));
		buf.append(date_buffer.get_buffer());
	}

	{
		for (const auto& iter : this->_headers) {
			buf.append(iter.first);
			buf.append(": ");
			buf.append(iter.second);
			buf.append("\r\n");
		}

		buf.append("\r\n");
	}
}

void server::server_response::_end(const node::buffer chunks[], size_t chunkcnt) {
	auto socket = this->socket();

	outgoing_message::_end(chunks, chunkcnt);

	if (this->_shutdown_on_end && socket) {
		socket->end();
	}

	// reset fields for the next response in a keepalive connection
	this->_status_code = 200;
	this->_shutdown_on_end = true;
}


server::server(node::loop& loop) : tcp::server(loop), _is_destroyed(std::make_shared<bool>(false)) {
	this->connection_callback.connect([this]() {
		const auto socket = node::make_shared<tcp::socket>(*this);
		this->accept(*socket);

		this->_clients.emplace_front(socket);
		
		const auto it = this->_clients.cbegin();
		const auto req = node::make_shared<server_request>(socket, HTTP_REQUEST);
		const auto res = node::make_shared<server_response>(socket);

		// TODO: (create and) use node::weak_ptr instead?
		const auto& _is_destroyed = this->_is_destroyed;
		socket->destroy_signal.connect([this, _is_destroyed, it, req, res]() {
			req->intrusive_ptr::destroy();
			res->intrusive_ptr::destroy();

			if (!*_is_destroyed) {
				this->_clients.erase(it);
			}
		});

		req->headers_complete_callback.connect([this, req, res](bool upgrade, bool keep_alive) {
			using namespace node::literals;

			/*
			 * TODO: A HTTP_REQUEST parser should continously run
			 * for the socket and *spawn* req/res pairs when needed.
			 * The same goes for http::request().
			 */
			//res->_shutdown_on_end = !keep_alive;
			//res->_reset();

			// RFC 2616 - 14.23
			if (!req->has_header("host"_view)) {
				res->set_status_code(400);
				res->end();
				return;
			}

			if (!this->request_callback.emit(req, res)) {
				res->set_status_code(500);
				res->end();
				return;
			}

			if (upgrade) {
				// TODO: externalize into upgrade adaptor classes
				/*
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
					res->set_header("connection"_view, "upgrade"_view);
					res->set_header("upgrade"_view,    "websocket"_view);
					res->set_header("sec-websocket-accept"_view, acceptHash);
					res->send_headers();
					return;
				}
				*/

				res->set_status_code(501);
				res->end();
			}
		});

		socket->resume();
	});
}

void server::_destroy() {
	*this->_is_destroyed = true;

	for (const auto& socket : this->_clients) {
		socket->destroy();
	}

	this->_clients.clear();
	this->request_callback.clear();

	node::tcp::server::_destroy();
}

} // namespace node
} // namespace http
