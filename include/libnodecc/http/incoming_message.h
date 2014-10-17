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
}


namespace node {
namespace http {

class incoming_message {
	friend class client_request;
	friend class server;

	NODE_ADD_CALLBACK(data, void, const node::buffer& buffer)
	NODE_ADD_CALLBACK(headers_complete, void, bool keep_alive)
	NODE_ADD_CALLBACK(end, void)
	NODE_ADD_CALLBACK(close, void)

public:
	explicit incoming_message(node::net::socket& socket, http_parser_type type);

	node::net::socket& socket;

	uint8_t http_version_major;
	uint8_t http_version_minor;

	uint8_t status_code;

	std::string method;
	std::string url;
	std::unordered_map<std::string, std::string> headers;

private:
	static int parser_on_url(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_field(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_value(http_parser* parser, const char* at, size_t length);
	static int parser_on_headers_complete(http_parser* parser);
	static int parser_on_body(http_parser* parser, const char* at, size_t length);
	static int parser_on_message_complete(http_parser* parser);

	void add_header_partials();
	void _close();

	std::string _partial_header_field;
	std::string _partial_header_value;

	http_parser _parser;
	const node::buffer* _parserBuffer;
};

} // namespace http
} // namespace node

#endif // nodecc_http_incoming_message_h
