#ifndef nodecc_http_server_response_h
#define nodecc_http_server_response_h

#include "request_response_proto.h"


namespace http {

class server_response : public request_response_proto {
public:
	explicit server_response(net::socket &socket);

	
	net::socket &socket;

	uint16_t statusCode;

private:
	void send_headers();
	bool socket_write(const util::buffer bufs[], size_t bufcnt);
};

} // namespace http

#endif // nodecc_http_server_response_h
