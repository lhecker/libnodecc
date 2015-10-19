#include "libnodecc/http/incoming_message.h"

#include <cctype>

#include "libnodecc/tcp/socket.h"


static const node::buffer_view method_map[] = {
#define XX(num, name, string) node::buffer_view(#string, strlen(#string)),
	HTTP_METHOD_MAP(XX)
#undef XX
};


namespace node {
namespace http {

url::url() : _state(state::uninitialized) {}

url::url(const node::buffer& url) : _url(url), _state(state::uninitialized) {
	this->_parse_url();
}

void url::set_url(const node::buffer& url) noexcept {
	this->_url = url;
	this->_params.clear();
	this->_state = state::uninitialized;
}

const node::buffer url::operator()() noexcept {
	return this->_url;
}

const node::buffer url::schema() noexcept {
	return this->_get(UF_SCHEMA);
}

const node::buffer url::host() noexcept {
	return this->_get(UF_HOST);
}

const node::buffer url::port() noexcept {
	return this->_get(UF_PORT);
}

const node::buffer url::path() noexcept {
	return this->_get(UF_PATH);
}

const node::buffer url::query() noexcept {
	return this->_get(UF_QUERY);
}

const node::buffer url::fragment() noexcept {
	return this->_get(UF_FRAGMENT);
}

const node::buffer url::userinfo() noexcept {
	return this->_get(UF_USERINFO);
}

uint16_t url::port_num() noexcept {
	this->_parse_url();
	return this->_parser.field_set & (1 << UF_PORT) ? this->_parser.port : 0;
}

bool url::has_param(const node::buffer& key) noexcept {
	this->_parse_params();
	return this->_params.find(key) != this->_params.cend();
}

const node::buffer& url::param(const node::buffer& key) noexcept {
	this->_parse_params();

	try {
		return this->_params.at(key);
	} catch (...) {
		static const node::buffer empty;
		return empty;
	}
}

const url::params_type& url::params() noexcept {
	this->_parse_params();
	return this->_params;
}

void url::clear() {
	this->_params.clear();
	this->_url.reset();
	this->_state = state::uninitialized;
}

bool url::_parse_url() {
	if (this->_state < state::url_parsed && this->_url) {
		this->_state = state::url_parsed;

		const int r = http_parser_parse_url(this->_url.data<char>(), this->_url.size(), false, &this->_parser);

		if (r != 0) {
			this->_state = state::error;
			return false;
		}
	}

	return true;
}

void url::_parse_params() {
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

node::buffer url::_get(uint_fast8_t type) noexcept {
	this->_parse_url();

	if (this->_parser.field_set & (1 << type)) {
		const auto& u = this->_parser.field_data[type];
		return this->_url.slice(u.off, u.off + u.len);
	} else {
		return node::buffer();
	}
}


incoming_message::incoming_message(const node::shared_ptr<node::tcp::socket>& socket, http_parser_type type) : _socket(socket), _is_websocket(UINT8_MAX) {
	static const http_parser_settings http_req_parser_settings = {
		nullptr,
		incoming_message::parser_on_url,
		nullptr,
		incoming_message::parser_on_header_field,
		incoming_message::parser_on_header_value,
		incoming_message::parser_on_headers_complete,
		incoming_message::parser_on_body,
		incoming_message::parser_on_message_complete,
		nullptr,
		nullptr,
	};


	http_parser_init(&this->_parser, type);
	this->_parser.data = this;

	this->_headers.max_load_factor(0.75);

	this->_socket->data_callback.connect([this](const node::buffer* bufs, size_t bufcnt) {
		for (size_t i = 0; i < bufcnt; i++) {
			const node::buffer* buf = bufs + i;

			this->_parser_buffer = buf;
			size_t nparsed = http_parser_execute(&this->_parser, &http_req_parser_settings, buf->data<char>(), buf->size());

			// TODO: handle upgrade
			if (this->_parser.upgrade == 1 || nparsed != buf->size()) {
				// prevent final http_parser_execute() in .end_callback.connect()?
				this->_socket->end();
			}
		}
	});

	this->_socket->end_signal.connect([this]() {
		http_parser_execute(&this->_parser, &http_req_parser_settings, nullptr, 0);
	});
}

const node::shared_ptr<node::tcp::socket>& incoming_message::socket() {
	return this->_socket;
}

const node::buffer& incoming_message::method() const {
	return this->_generic_value;
}

bool incoming_message::has_header(const node::hashed_buffer& key) const {
	return this->_headers.find(key) != this->_headers.cend();
}

const node::buffer& incoming_message::header(const node::hashed_buffer& key) const {
	try {
		return this->_headers.at(key);
	} catch (...) {
		static const node::buffer empty;
		return empty;
	}
}

uint16_t incoming_message::status_code() const {
	return this->_status_code;
}

uint8_t incoming_message::http_version_major() const {
	return this->_http_version_major;
}

uint8_t incoming_message::http_version_minor() const {
	return this->_http_version_minor;
}

bool incoming_message::is_websocket_request() {
	if (this->_is_websocket == UINT8_MAX) {
		if (this->_parser.upgrade != 0) {
			using namespace node::literals;

			const auto upgradeField = this->header("upgrade"_view);
			const auto versionField = this->header("sec-websocket-version"_view);
			const auto keyField = this->header("sec-websocket-key"_view);

			if (upgradeField.equals("websocket"_view) &&
				versionField.equals("13"_view) &&
				keyField)
			{
				this->_is_websocket = 1;
				return true;
			}
		}

		this->_is_websocket = 0;
	}

	return this->_is_websocket == 1;
}

void incoming_message::_resume() {
	if (this->_socket) {
		this->_socket->resume();
	}
}

void incoming_message::_pause() {
	if (this->_socket) {
		this->_socket->pause();
	}
}

void incoming_message::destroy() {
	if (this->_socket) {
		this->_socket->destroy();
	}
}

int incoming_message::parser_on_url(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->_generic_value.append(at, length);

	return 0;
}

int incoming_message::parser_on_header_field(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->_add_header_partials();
	self->_partial_header_field.append(self->_buffer(at, length));

	return 0;
}

int incoming_message::parser_on_header_value(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->_add_header_partials();
	self->_partial_header_value.append(self->_buffer(at, length));

	return 0;
}

int incoming_message::parser_on_headers_complete(http_parser* parser) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->_add_header_partials();

	if (parser->type == HTTP_REQUEST) {
		self->_http_version_major = static_cast<uint8_t>(parser->http_major);
		self->_http_version_minor = static_cast<uint8_t>(parser->http_minor);

		self->url.set_url(self->_generic_value);

		self->_generic_value.reset(method_map[parser->method], node::buffer_flags::weak);
	} else {
		// HTTP_RESPONSE
		self->_status_code = static_cast<uint16_t>(parser->status_code);
	}

	self->headers_complete_callback.emit(parser->upgrade != 0, http_should_keep_alive(parser) != 0);

	return 0;
}

int incoming_message::parser_on_body(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	if (self->data_callback) {
		const auto buf = self->_buffer(at, length);
		self->data_callback.emit(&buf, 1);
	}

	return 0;
}

int incoming_message::parser_on_message_complete(http_parser* parser) {
	auto self = static_cast<incoming_message*>(parser->data);

	if (self->_is_websocket != 1) {
		self->end_signal.emit();
		self->_generic_value.reset();
		self->_headers.clear();
		self->url.clear();
	}

	return 0;
}

void incoming_message::_add_header_partials() {
	if (this->_partial_header_field && this->_partial_header_value) {
		// manual tolower() to workaround some compiler hickups & issues
		for (uint8_t* data = this->_partial_header_field.begin(), *end = this->_partial_header_field.end(); data < end; data++) {
			const uint8_t ch = *data;

			if (ch >= 0x41 && ch <= 0x5a) {
				*data = ch + 0x20;
			}
		}

		const auto iter = this->_headers.emplace(this->_partial_header_field, this->_partial_header_value);

		if (!iter.second) {
			auto& existingValue = iter.first->second;
			existingValue.append(", ");
			existingValue.append(this->_partial_header_value);
		}

		this->_partial_header_field.reset();
		this->_partial_header_value.reset();
	}
}

node::buffer incoming_message::_buffer(const char* at, size_t length) {
	const size_t start = at - this->_parser_buffer->data<char>();
	return this->_parser_buffer->slice(start, start + length);
}

void incoming_message::_destroy() {
	this->_socket.reset();

	this->headers_complete_callback.clear();

	node::stream::readable<incoming_message, int, node::buffer>::_destroy();
	node::intrusive_ptr::_destroy();
}

} // namespace node
} // namespace http
