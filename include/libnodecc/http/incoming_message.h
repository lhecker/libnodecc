#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <functional>
#include <string>
#include <unordered_map>


struct http_parser;

namespace net {
	class socket;
}

namespace util {
	class buffer;
}


namespace http {

class incoming_message {
public:
	typedef std::function<void()> on_complete_t;
	typedef std::function<void(const util::buffer &buffer)> on_body_t;


	explicit incoming_message(net::socket &socket);

	~incoming_message();


	net::socket &socket;

	uint8_t http_version_major;
	uint8_t http_version_minor;

	std::string method;
	std::string url;
	std::unordered_map<std::string, std::string> headers;

	on_complete_t on_headers_complete;
	on_complete_t on_message_complete;
	on_body_t     on_body;


private:
	static int parser_on_url(http_parser *parser, const char *at, size_t length);
	static int parser_on_header_field(http_parser *parser, const char *at, size_t length);
	static int parser_on_header_value(http_parser *parser, const char *at, size_t length);
	static int parser_on_headers_complete(http_parser *parser);
	static int parser_on_body(http_parser *parser, const char *at, size_t length);
	static int parser_on_message_complete(http_parser *parser);

	std::string _partialHeaderField;
	std::string *_partialHeaderValue;

	http_parser *_parser;
	const util::buffer *_parserBuffer;
};

} // namespace http

#endif // nodecc_http_incoming_message_h
