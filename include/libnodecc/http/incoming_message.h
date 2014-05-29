#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <cstdint>
#include <functional>
#include <unordered_map>


struct uv_buf_t;
struct http_parser;

namespace net {
class socket;
}


namespace http {

class incoming_message {
public:
	typedef std::function<void()> on_complete_t;
	typedef std::function<void(uv_buf_t)> on_body_t;


	explicit incoming_message(net::socket &socket);


	net::socket &socket;

	uint8_t http_version_major;
	uint8_t http_version_minor;

	std::string method;
	std::string url;
	std::unordered_map<std::string, std::string> headers;

	on_complete_t on_headers_complete;
	on_complete_t on_message_complete;
	on_body_t     on_body;


	// *** private ***
	std::string _partialHeaderField;
	std::string *_partialHeaderValue;

	http_parser *_parser;
};

} // namespace http

#endif // nodecc_http_incoming_message_h
