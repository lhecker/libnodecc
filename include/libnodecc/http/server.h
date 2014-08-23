#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <functional>
#include <unordered_set>

#include "../net/server.h"
#include "incoming_message.h"
#include "server_response.h"


namespace http {

class server : public net::server {
public:
	typedef std::function<void(http::incoming_message &req, http::server_response &res)> on_request_t;


	explicit server();

	using net::server::init;


	std::unordered_set<net::socket> clients;

	on_request_t on_request;
};

} // namespace http

#endif // nodecc_http_server_h
