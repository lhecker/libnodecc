#ifndef nodecc_http_server_response_h
#define nodecc_http_server_response_h

#include "request_response_proto.h"


namespace http {

class server_response : public request_response_proto {
public:
	explicit server_response(net::socket &socket);

	bool end();

	
	net::socket &socket;

	uint16_t statusCode;

private:
	// http::server needs exclusive access to _close_on_end
	friend class server;

	void send_headers();
	bool socket_write(const util::buffer bufs[], size_t bufcnt);

	bool _close_on_end;
};

} // namespace http

#endif // nodecc_http_server_response_h
