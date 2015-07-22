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

class url {
public:
	explicit url() : _parser_stale(false), _params_stale(false) {}
	explicit url(const node::buffer& url) : _url(url), _parser_stale(true), _params_stale(true) {}

	void set_url(const node::buffer& url) noexcept {
		this->_url = url;
		this->_params.clear();
		this->_parser_stale = true;
		this->_params_stale = true;
	}

	const node::buffer operator()() noexcept {
		return this->_url;
	}

	const node::buffer schema() noexcept {
		return this->_get(UF_SCHEMA);
	}

	const node::buffer host() noexcept {
		return this->_get(UF_HOST);
	}

	const node::buffer port() noexcept {
		return this->_get(UF_PORT);
	}

	const node::buffer path() noexcept {
		return this->_get(UF_PATH);
	}

	const node::buffer query() noexcept {
		return this->_get(UF_QUERY);
	}

	const node::buffer fragment() noexcept {
		return this->_get(UF_FRAGMENT);
	}

	const node::buffer userinfo() noexcept {
		return this->_get(UF_USERINFO);
	}

	uint16_t port_num() noexcept {
		this->_parse_url();

		if (this->_parser.field_set & (1 << UF_PORT)) {
			return this->_parser.port;
		} else {
			return 0;
		}
	}

	bool has_param(const node::buffer_view& key) noexcept {
		return this->_params.find(key) != this->_params.cend();
	}

	const node::buffer& param(const node::buffer_view& key) noexcept {
		try {
			return this->_params.at(key);
		} catch (...) {
			static const node::buffer empty;
			return empty;
		}
	}

private:
	bool _parse_url() {
		if (!this->_parser_stale) {
			return true;
		}

		this->_parser_stale = false;

		const int r = http_parser_parse_url(this->_url.data<char>(), this->_url.size(), false, &this->_parser);

		if (r != 0) {
			return false;
		}
	}

	bool _parse_params() {
		if (this->_params_stale && this->_parse_url()) {
			this->_params_stale = false;

			const node::buffer query = this->query();
			uint8_t* data = query.data();
			const size_t size = query.size();
			size_t current_amp_pos = 0;
			size_t current_eq_pos = 0;

			for (size_t i = 0; i < size; i++) {
				const auto ch = data[i];

				if (ch == '&') {
					current_amp_pos = i;

					// TODO: split value with (current_eq_pos, i)

					current_eq_pos = 0;
				} else if (ch == '=' && current_eq_pos == 0) {
					current_eq_pos = i;

					// TODO: split field with (current_amp_pos, i)

					current_amp_pos = 0;
				}
			}
		}
	}

	node::buffer _get(uint_fast8_t type) noexcept {
		this->_parse_url();

		if (this->_parser.field_set & (1 << type)) {
			const auto& u = this->_parser.field_data[type];
			return this->_url.slice(u.off, u.off + u.len);
		} else {
			return node::buffer();
		}
	}

	std::unordered_map<node::buffer, node::buffer> _params;
	node::buffer _url;
	http_parser_url _parser;
	bool _parser_stale;
	bool _params_stale;
};


class incoming_message {
	friend class client_request;
	friend class server;

	// for node::http::server/client_request
	node::event<void()> on_end;

public:
	node::event<void(bool upgrade, bool keep_alive)> on_headers_complete;
	node::event<void(const node::buffer& buffer)> on_data;
	node::event<void()> on_close;

	explicit incoming_message(node::net::socket& socket, http_parser_type type);

	node::net::socket& socket();

	const node::buffer& method() const;
	node::http::url url;

	bool has_header(const node::buffer_view& key) const;
	const node::buffer& header(const node::buffer_view& key) const;

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

	std::unordered_map<node::buffer, node::mutable_buffer> _headers;

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
