#ifndef nodecc_http_incoming_message_h
#define nodecc_http_incoming_message_h

#include <array>
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
class outgoing_message;
}

}


namespace node {
namespace http {

class url {
public:
	typedef std::unordered_map<node::buffer, node::buffer> params_type;


	explicit url() : _state(state::uninitialized) {}

	explicit url(const node::buffer& url) : _url(url), _state(state::uninitialized) {
		this->_parse_url();
	}

	void set_url(const node::buffer& url) noexcept {
		this->_url = url;
		this->_params.clear();
		this->_state = state::uninitialized;
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
		return this->_parser.field_set & (1 << UF_PORT) ? this->_parser.port : 0;
	}

	bool has_param(const node::buffer_view& key) noexcept {
		this->_parse_params();
		return this->_params.find(key) != this->_params.cend();
	}

	const node::buffer& param(const node::buffer_view& key) noexcept {
		this->_parse_params();

		try {
			return this->_params.at(key);
		} catch (...) {
			static const node::buffer empty;
			return empty;
		}
	}

	const params_type& params() noexcept {
		this->_parse_params();
		return this->_params;
	}

private:
	enum state : uint8_t {
		uninitialized = 0,
		url_parsed,
		params_parsed,
		error,
	};


	bool _parse_url() {
		if (this->_state < state::url_parsed && this->_url) {
			this->_state = state::url_parsed;

			const int r =  http_parser_parse_url(this->_url.data<char>(), this->_url.size(), false, &this->_parser);

			if (r != 0) {
				this->_state = state::error;
				return false;
			}
		}

		return true;
	}

	void _parse_params() {
		if (this->_state < state::params_parsed && this->_parse_url()) {
			this->_state = state::params_parsed;

			const node::buffer& query = this->query();

			if (query) {
				uint8_t* data = query.data();
				const size_t size = query.size();

				size_t current_amp_pos = size_t(-1);
				size_t current_eq_pos = 0;
				size_t i = 0;
				node::buffer field;

				for (; i < size; i++) {
					const auto ch = data[i];

					if (ch == '&') {
						if ((i - current_eq_pos) > 1 && field) {
							this->_params.emplace(std::move(field), query.slice(current_eq_pos + 1, i));
						}

						current_amp_pos = i;
						current_eq_pos = 0;
					} else if (ch == '=' && current_eq_pos == 0) {
						if ((i - current_amp_pos) > 1) {
							field = query.slice(current_amp_pos + 1, i);
						}

						current_amp_pos = 0;
						current_eq_pos = i;
					}
				}

				// try to insert leftovers
				if ((i - current_eq_pos) > 1 && field) {
					this->_params.emplace(std::move(field), query.slice(current_eq_pos + 1, i));
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

	params_type _params;
	node::buffer _url;
	http_parser_url _parser;
	uint8_t _state;
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
