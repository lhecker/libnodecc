#ifndef nodecc_http_client_request_h
#define nodecc_http_client_request_h

#include "request_response_proto.h"
#include "incoming_message.h"

#include "../net/socket.h"


namespace http {

class client_request : public http::request_response_proto {
public:
	typedef std::function<void(http::client_request& req, http::incoming_message& res)> on_connect_t;
	typedef std::function<void()> on_error_t;


	explicit client_request();

	bool init(uv::loop& loop, const std::string& hostname, const uint16_t port, const on_connect_t& cb);

	void shutdown();
	void close();

	const std::string& method() const;
	const std::string& path() const;
	const std::string& hostname() const;
	uint16_t port() const;

	void set_method(const std::string& method);
	void set_path(const std::string& path);


	on_connect_t on_connect;
	on_error_t on_error;

private:
	void send_headers();
	bool socket_write(const util::buffer bufs[], size_t bufcnt);

	net::socket _socket;
	http::incoming_message _incoming_message;
	std::string _method;
	std::string _path;
	std::string _hostname;
	uint16_t _port;
};

} // namespace http

#endif // nodecc_http_client_request_h
