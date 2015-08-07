#ifndef nodecc_http_request_h
#define nodecc_http_request_h

#include "incoming_message.h"
#include "outgoing_message.h"

#include "../net/socket.h"


namespace node {
namespace http {
namespace client {

namespace detail {
class request;
class response;
}

typedef std::shared_ptr<detail::request> request;
typedef std::shared_ptr<detail::response> response;
typedef std::function<void(int err, const request& req, const response& res)> on_connect_t;


namespace detail {

#define NODE_HTTP_REQUEST_GENERATOR_SIGNATURE \
	std::shared_ptr<node::net::socket> _generate(node::loop& loop, const node::buffer& host, const node::buffer& method, const node::buffer& path, const client::on_connect_t& cb)

static NODE_HTTP_REQUEST_GENERATOR_SIGNATURE;

class request : public node::http::outgoing_message {
	friend NODE_HTTP_REQUEST_GENERATOR_SIGNATURE;

public:
	explicit request(const std::shared_ptr<node::net::socket>& socket, const node::buffer& host, const node::buffer& method, const node::buffer& path);

	~request() {
		printf("~request\n");
	}

private:
	void compile_headers(node::mutable_buffer& buf) override;

	node::buffer _host;
	node::buffer _method;
	node::buffer _path;
};

class response : public node::http::incoming_message {
	friend NODE_HTTP_REQUEST_GENERATOR_SIGNATURE;

public:
	explicit response(const std::shared_ptr<node::net::socket>& socket);

	~response() {
		printf("~response\n");
	}
};

} // namespace detail
} // namespace client


void request(node::loop& loop, const node::buffer& method, const node::buffer& url, const client::on_connect_t& cb);
void request(node::loop& loop, const sockaddr& addr, const node::buffer& host, const node::buffer& method, const node::buffer& path, const client::on_connect_t& cb);

} // namespace http
} // namespace node

#endif // nodecc_http_request_h
