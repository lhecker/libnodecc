#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <functional>
#include <memory>

#include "../net/server.h"
#include "incoming_message.h"
#include "server_response.h"


namespace node {
namespace http {

class server : public node::net::server {
public:
	typedef std::shared_ptr<node::http::incoming_message> request;
	typedef std::shared_ptr<node::http::server_response> response;

	node::callback<void(const node::http::server::request& req, const node::http::server::response& res)> request_callback;

public:
	explicit server();
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
