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
	NODE_ADD_CALLBACK(request, http::incoming_message& req, http::server_response& res)

public:
	explicit server();

private:
	// TODO: convert it to an intrusive double-linked list
	std::unordered_set<node::net::socket, std::hash<node::net::socket::handle_type>> clients;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
