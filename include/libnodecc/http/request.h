#ifndef nodecc_http_request_h
#define nodecc_http_request_h

#include "incoming_message.h"
#include "outgoing_message.h"

#include "../net/socket.h"


namespace node {
namespace http {
namespace client {
namespace detail {

class request : public node::http::outgoing_message {
public:
	explicit request(const std::shared_ptr<node::net::socket>& socket, const node::buffer& host, const node::buffer& method, const node::buffer& path);

private:
	void compile_headers(node::mutable_buffer& buf) override;

	node::buffer _host;
	node::buffer _method;
	node::buffer _path;
};

class response : public node::http::incoming_message {
public:
	explicit response(const std::shared_ptr<node::net::socket>& socket);
};

} // namespace detail

typedef std::shared_ptr<detail::request> request;
typedef std::shared_ptr<detail::response> response;
typedef std::function<void(int err, const request& req, const response& res)> on_connect_t;

} // namespace client


void request(node::loop& loop, const node::buffer& method, const node::buffer& url, client::on_connect_t cb);
void request(node::loop& loop, const addrinfo& addr, const node::buffer& host, const node::buffer& method, const node::buffer& path, client::on_connect_t cb);

} // namespace http
} // namespace node

#endif // nodecc_http_request_h
