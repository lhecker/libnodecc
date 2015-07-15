#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <functional>
#include <http-parser/http_parser.h>
#include <unordered_map>

#include "../buffer.h"
#include "../event.h"


namespace node {

class buffer;

namespace net {
class socket;
}

namespace http {
class request_response_proto;
}

}


namespace node {
namespace http {

class incoming_message {
	friend class client_request;
	friend class server;

public:
	node::event<void(bool upgrade, bool keep_alive)> on_headers_complete;
	node::event<void(const node::buffer& buffer)> on_data;
	node::event<void()> on_close;

private:
	// for node::http::server/client_request
	node::event<void()> on_end;

public:
	typedef std::unordered_map<node::buffer, node::mutable_buffer> headers_t;


	explicit incoming_message(node::net::socket& socket, http_parser_type type);

	node::net::socket& socket();

	node::buffer method() const;
	node::buffer url() const;

	bool has_header(const node::buffer_view key) const;
	node::buffer header(const node::buffer_view key) const;

	uint16_t status_code() const;
	uint8_t http_version_major() const;
	uint8_t http_version_minor() const;

	bool is_websocket_request();

private:
	static int parser_on_url(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_field(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_value(http_parser* parser, const char* at, size_t length);
	static int parser_on_headers_complete(http_parser* parser);
	static int parser_on_body(http_parser* parser, const char* at, size_t length);
	static int parser_on_message_complete(http_parser* parser);

	void _add_header_partials();
	node::buffer _buffer(const char* at, size_t length);
	void _close();

	node::net::socket& _socket;

	node::buffer _method;
	node::mutable_buffer _url;
	headers_t _headers;

	node::mutable_buffer _partial_header_field;
	node::mutable_buffer _partial_header_value;

	const node::buffer* _parser_buffer;
	http_parser _parser;

	uint16_t _status_code;
	uint8_t _http_version_major;
	uint8_t _http_version_minor;
	uint8_t _is_websocket;
};

} // namespace http
} // namespace node

#endif // nodecc_http_incoming_message_h
