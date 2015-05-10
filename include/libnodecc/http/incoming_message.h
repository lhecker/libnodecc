#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <functional>
#include <http-parser/http_parser.h>
#include <string>
#include <unordered_map>

#include "../common.h"


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

	NODE_CALLBACK_ADD(public, headers_complete, void, bool upgrade, bool keep_alive)
	NODE_CALLBACK_ADD(public, data, void, const node::buffer& buffer)
	NODE_CALLBACK_ADD(public, close, void)

	// for node::http::server/client_request
	NODE_CALLBACK_ADD(private, end, void)

public:
	typedef std::unordered_map<std::string, std::string> headers_t;


	explicit incoming_message(node::net::socket& socket, http_parser_type type);

	node::net::socket& socket();

	const std::string& method() const;
	const std::string& url() const;
	const headers_t& headers() const;
	uint8_t http_version_major() const;
	uint8_t http_version_minor() const;
	uint8_t status_code() const;

	bool is_websocket_request();

private:
	static int parser_on_url(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_field(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_value(http_parser* parser, const char* at, size_t length);
	static int parser_on_headers_complete(http_parser* parser);
	static int parser_on_body(http_parser* parser, const char* at, size_t length);
	static int parser_on_message_complete(http_parser* parser);

	void add_header_partials();
	void _close();

	node::net::socket& _socket;

	std::string _method;
	std::string _url;
	headers_t _headers;

	std::string _partial_header_field;
	std::string _partial_header_value;

	const node::buffer* _parserBuffer;
	http_parser _parser;

	uint8_t _http_version_major;
	uint8_t _http_version_minor;
	uint8_t _status_code;
	uint8_t _is_websocket;
};

} // namespace http
} // namespace node

#endif // nodecc_http_incoming_message_h
