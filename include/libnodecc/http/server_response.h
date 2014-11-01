#ifndef nodecc_http_server_response_h
#define nodecc_http_server_response_h

#include "request_response_proto.h"


namespace node {
namespace http {

class server_response : public node::http::request_response_proto {
public:
	explicit server_response(node::net::socket& socket);

	using http::request_response_proto::end;
	bool end(const node::buffer bufs[], size_t bufcnt);

	node::net::socket& socket() const;

	uint16_t status_code() const;
	void set_status_code(uint16_t code);

private:
	// http::server needs exclusive access to _shutdown_on_end
	friend class server;

	void send_headers();
	bool socket_write(const node::buffer bufs[], size_t bufcnt);

	node::net::socket& _socket;
	uint16_t _status_code;
	bool _shutdown_on_end;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_response_h
