#include "libnodecc/http/incoming_message.h"

#include <algorithm>
#include <cctype>

#include "libnodecc/net/socket.h"


namespace node {
namespace http {

incoming_message::incoming_message(net::socket& socket, http_parser_type type) : _socket(socket), _is_websocket(UINT8_MAX) {
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

	socket.on_data([this](const node::buffer* bufs, size_t bufcnt) {
		for (size_t i = 0; i < bufcnt; i++) {
			const node::buffer* buf = bufs + i;

			this->_parser_buffer = buf;
			size_t nparsed = http_parser_execute(&this->_parser, &http_req_parser_settings, buf->data<char>(), buf->size());

			// TODO: handle upgrade
			if (this->_parser.upgrade == 1 || nparsed != buf->size()) {
				// prevent final http_parser_execute() in .on_end()?
				this->_socket.end();
			}
		}
	});

	socket.on_end([this]() {
		http_parser_execute(&this->_parser, &http_req_parser_settings, nullptr, 0);
	});
}

node::net::socket& incoming_message::socket() {
	return this->_socket;
}

node::buffer incoming_message::method() const {
	return this->_method;
}

node::buffer incoming_message::url() const {
	return this->_url;
}

bool incoming_message::has_header(const node::buffer_view key) const {
	return this->_headers.find(key) != this->_headers.cend();
}

node::buffer incoming_message::header(const node::buffer_view key) const {
	try {
		return this->_headers.at(key);
	} catch (...) {
		return node::buffer();
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
			const auto upgradeField = this->header("upgrade");
			const auto versionField = this->header("sec-websocket-version");
			const auto keyField = this->header("sec-websocket-key");

			if (upgradeField.compare("websocket") == 0 &&
				versionField.compare("13") == 0 &&
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

int incoming_message::parser_on_url(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->_url.append(at, length);

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
		self->_method.reset(http_method_str(static_cast<http_method>(parser->method)), node::weak);
	} else {
		// HTTP_RESPONSE
		self->_status_code = static_cast<uint16_t>(parser->status_code);
	}

	self->on_headers_complete.emit(parser->upgrade != 0, http_should_keep_alive(parser) != 0);

	return 0;
}

int incoming_message::parser_on_body(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	if (self->on_data) {
		self->on_data.emit(self->_buffer(at, length));
	}

	return 0;
}

int incoming_message::parser_on_message_complete(http_parser* parser) {
	auto self = static_cast<incoming_message*>(parser->data);

	if (self->_is_websocket != 1) {
		self->_method.reset();
		self->_url.reset();
		self->_headers.clear();
		self->on_end.emit();
	}

	return 0;
}

void incoming_message::_add_header_partials() {
	if (this->_partial_header_field && this->_partial_header_value) {
		//std::transform(this->_partial_header_field.begin(), this->_partial_header_field.end(), this->_partial_header_field.begin(), std::tolower);

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
	const ssize_t start = (uint8_t*)at - this->_parser_buffer->get();
	return this->_parser_buffer->slice(start, start + length);
}

void incoming_message::_close() {
	this->on_close.emit();
	this->on_data(nullptr);
	this->on_close(nullptr);
	this->on_end(nullptr);

	// delete this last since node::http::server uses this callback to store a strong reference to the req_res_pack
	this->on_headers_complete(nullptr);
}

} // namespace node
} // namespace http
