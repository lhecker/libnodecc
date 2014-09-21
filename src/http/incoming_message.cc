#include "libnodecc/http/incoming_message.h"

#include <algorithm>
#include <cctype>

#include "libnodecc/net/socket.h"


namespace node {
namespace http {

incoming_message::incoming_message(net::socket& socket, http_parser_type type) : socket(socket) {
	static const http_parser_settings http_req_parser_settings = {
		nullptr,
		incoming_message::parser_on_url,
		nullptr,
		incoming_message::parser_on_header_field,
		incoming_message::parser_on_header_value,
		incoming_message::parser_on_headers_complete,
		incoming_message::parser_on_body,
		incoming_message::parser_on_message_complete,
	};


	http_parser_init(&this->_parser, type);
	this->_parser.data = this;

	this->headers.max_load_factor(0.75);

	socket.on_read([this](int err, const node::buffer& buffer) {
		if (err) {
			if (err == UV_EOF) {
				http_parser_execute(&this->_parser, &http_req_parser_settings, nullptr, 0);
			}

			this->socket.close();
		} else {
			this->_parserBuffer = &buffer;
			size_t nparsed = http_parser_execute(&this->_parser, &http_req_parser_settings, buffer, buffer.size());

			// TODO: handle upgrade
			if (this->_parser.upgrade == 1 || nparsed != buffer.size()) {
				this->socket.shutdown();
			}
		}
	});
}

int incoming_message::parser_on_url(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);
	self->url.append(at, length);
	return 0;
}

int incoming_message::parser_on_header_field(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);
	self->add_header_partials();
	self->_partial_header_field.append(at, length);
	return 0;
}

int incoming_message::parser_on_header_value(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);
	self->add_header_partials();
	self->_partial_header_value.append(at, length);
	return 0;
}

int incoming_message::parser_on_headers_complete(http_parser* parser) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->add_header_partials();

	if (parser->type == HTTP_REQUEST) {
		self->http_version_major = static_cast<uint8_t>(parser->http_major);
		self->http_version_minor = static_cast<uint8_t>(parser->http_minor);
		self->method = http_method_str(static_cast<http_method>(parser->method));
	} else {
		// HTTP_RESPONSE
		self->status_code = static_cast<uint16_t>(parser->status_code);
	}

	self->emit_headers_complete(http_should_keep_alive(parser));

	return 0;
}

int incoming_message::parser_on_body(http_parser* parser, const char* at, size_t length) {
	auto self = static_cast<incoming_message*>(parser->data);

	if (self->_on_data) {
		ssize_t start = (uint8_t*)at - self->_parserBuffer->get();
		self->_on_data(self->_parserBuffer->slice(start, start + length));
	}

	return 0;
}

int incoming_message::parser_on_message_complete(http_parser* parser) {
	auto self = static_cast<incoming_message*>(parser->data);

	self->emit_end();

	self->method.clear();
	self->url.clear();
	self->headers.clear();

	return 0;
}

void incoming_message::add_header_partials() {
	if (!this->_partial_header_field.empty() && !this->_partial_header_value.empty()) {
		std::transform(this->_partial_header_field.begin(), this->_partial_header_field.end(), this->_partial_header_field.begin(), std::tolower);

		const auto iter = this->headers.emplace(this->_partial_header_field, this->_partial_header_value);

		if (!iter.second) {
			std::string& existingValue = iter.first->second;
			existingValue.append(", ");
			existingValue.append(this->_partial_header_value);
		}

		this->_partial_header_field.clear();
		this->_partial_header_value.clear();
	}
}

} // namespace node
} // namespace http
