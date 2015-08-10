#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <array>
#include <functional>
#include <http-parser/http_parser.h>
#include <unordered_map>

#include "../buffer.h"
#include "../net/socket.h"
#include "../stream.h"


namespace node {

class buffer;

namespace net {
class socket;
}

namespace http {
class outgoing_message;
}

}


namespace node {
namespace http {

class url {
public:
	typedef std::unordered_map<node::hashed_buffer, node::buffer> params_type;


	explicit url();

	explicit url(const node::buffer& url);

	void set_url(const node::buffer& url) noexcept;

	const node::buffer operator()() noexcept;
	const node::buffer schema() noexcept;
	const node::buffer host() noexcept;
	const node::buffer port() noexcept;
	const node::buffer path() noexcept;
	const node::buffer query() noexcept;
	const node::buffer fragment() noexcept;
	const node::buffer userinfo() noexcept;

	uint16_t port_num() noexcept;

	bool has_param(const node::buffer& key) noexcept;
	const node::buffer& param(const node::buffer& key) noexcept;
	const params_type& params() noexcept;

private:
	enum state : uint8_t {
		uninitialized = 0,
		url_parsed,
		params_parsed,
		error,
	};

	bool _parse_url();
	void _parse_params();
	node::buffer _get(uint_fast8_t type) noexcept;

	params_type _params;
	node::buffer _url;
	http_parser_url _parser;
	uint8_t _state;
};


class incoming_message : public node::stream::readable<int, node::buffer> {
	friend class server;

public:
	explicit incoming_message(const node::shared_object<node::net::socket>& socket, http_parser_type type);

	const node::shared_object<node::net::socket>& socket();

	const node::buffer& method() const;
	node::http::url url;

	bool has_header(const node::hashed_buffer& key) const;
	const node::buffer& header(const node::hashed_buffer& key) const;

	uint16_t status_code() const;
	uint8_t http_version_major() const;
	uint8_t http_version_minor() const;

	bool is_websocket_request();

	void resume() override;
	void pause() override;

	void destroy();

	node::signal<void()> destroy_signal;
	node::callback<void(bool upgrade, bool keep_alive)> headers_complete_callback;

protected:
	void _destroy();

private:
	static int parser_on_url(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_field(http_parser* parser, const char* at, size_t length);
	static int parser_on_header_value(http_parser* parser, const char* at, size_t length);
	static int parser_on_headers_complete(http_parser* parser);
	static int parser_on_body(http_parser* parser, const char* at, size_t length);
	static int parser_on_message_complete(http_parser* parser);

	void _add_header_partials();
	node::buffer _buffer(const char* at, size_t length);


	node::shared_object<node::net::socket> _socket;

	std::unordered_map<node::hashed_buffer, node::mutable_buffer> _headers;

	node::mutable_buffer _generic_value;
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
