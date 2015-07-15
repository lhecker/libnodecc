#ifndef nodecc_http_client_request_h
#define nodecc_http_client_request_h

#include "incoming_message.h"
#include "request_response_proto.h"

#include "../net/socket.h"


namespace node {
namespace http {

class client_request : public node::http::request_response_proto {
public:
	node::event<void(node::http::client_request& req, node::http::incoming_message& res)> on_connect;
	node::event<void()> on_error;

public:
	explicit client_request();

	bool init(node::loop& loop, const sockaddr& addr, const std::string& hostname, decltype(on_connect)::function_type cb);
	bool init(node::loop& loop, const std::string& url, decltype(on_connect)::function_type cb);

	void shutdown();
	void close();

	const std::string& method() const;
	const std::string& path() const;
	const std::string& hostname() const;

	void set_method(const std::string& method);
	void set_path(const std::string& path);

	node::net::socket& socket();

private:
	void compile_headers(node::mutable_buffer& buf) override;
	void socket_write(const node::buffer bufs[], size_t bufcnt) override;

	node::net::socket _socket;
	node::http::incoming_message _incoming_message;
	std::string _method;
	std::string _path;
	std::string _hostname;
};

} // namespace http
} // namespace node

#endif // nodecc_http_client_request_h
