#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <functional>
#include <http-parser/http_parser.h>
#include <string>
#include <unordered_map>


namespace node {
class buffer;

namespace net {
class socket;
}
}


namespace node {
namespace http {

class incoming_message {
public:
	typedef std::function<void(const node::buffer& buffer)> on_data_t;
	typedef std::function<void(bool keep_alive)> on_headers_complete_t;
	typedef std::function<void()> on_nil_t;


	explicit incoming_message(node::net::socket& socket, http_parser_type type);


	node::net::socket& socket;

	uint8_t http_version_major;
	uint8_t http_version_minor;

	uint8_t status_code;

	std::string method;
	std::string url;
	std::unordered_map<std::string, std::string> headers;

	on_data_t on_data;
	on_nil_t  on_end;
	on_nil_t  on_close;

private:
	friend class server;

	static int parser_on_url(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_field(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_value(http_parser* parser, const char* at, size_t length);
	static int parser_on_headers_complete(http_parser* parser);
	static int parser_on_body(http_parser* parser, const char* at, size_t length);
	static int parser_on_message_complete(http_parser* parser);

	void add_header_partials();

	on_headers_complete_t _on_headers_complete;

	std::string _partial_header_field;
	std::string _partial_header_value;

	http_parser _parser;
	const node::buffer* _parserBuffer;
};

} // namespace http
} // namespace node

#endif // nodecc_http_incoming_message_h
