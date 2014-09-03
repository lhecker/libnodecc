#ifndef nodecc_http_client_request_h
#define nodecc_http_client_request_h

#include "request_response_proto.h"

#include "../net/socket.h"


namespace http {

class client_request : public request_response_proto, public net::socket {
public:
	typedef std::function<void()> on_response_t;


	explicit client_request();


	std::string method;
	std::string path;
	std::string hostname;

	on_response_t on_response;

private:
	void send_headers();
	bool socket_write(const util::buffer bufs[], size_t bufcnt);
};

} // namespace http

#endif // nodecc_http_client_request_h
