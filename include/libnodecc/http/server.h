#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <functional>
#include <unordered_set>

#include "../net/server.h"
#include "incoming_message.h"
#include "server_response.h"


namespace node {
namespace http {

class server : public node::net::server {
public:
	typedef std::function<void(http::incoming_message& req, http::server_response& res)> on_request_t;


	explicit server();


	// TODO: convert it to an intrusive double-linked list
	std::unordered_set<node::net::socket, std::hash<node::net::socket::handle_type>> clients;

	on_request_t on_request;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
