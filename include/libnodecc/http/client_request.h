#ifndef nodecc_http_client_request_h
#define nodecc_http_client_request_h

#include "incoming_message.h"
#include "request_response_proto.h"

#include "../net/socket.h"


namespace node {
namespace http {

class client_request : public node::http::request_response_proto {
	NODE_ADD_CALLBACK(public, connect, void, node::http::client_request& req, node::http::incoming_message& res)
	NODE_ADD_CALLBACK(public, error, void)

	
	explicit client_request();

	bool init(node::loop& loop, const sockaddr& addr, const std::string& hostname, on_connect_t cb);
	bool init(node::loop& loop, const std::string& url, on_connect_t cb);

	void shutdown();
	void close();

	const std::string& method() const;
	const std::string& path() const;
	const std::string& hostname() const;

	void set_method(const std::string& method);
	void set_path(const std::string& path);

	const node::net::socket& socket() const;

private:
	void send_headers();
	bool socket_write(const node::buffer bufs[], size_t bufcnt);

	node::net::socket _socket;
	node::http::incoming_message _incoming_message;
	std::string _method;
	std::string _path;
	std::string _hostname;
};

} // namespace http
} // namespace node

#endif // nodecc_http_client_request_h
