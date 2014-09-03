#include "libnodecc/http/incoming_message.h"

#include <algorithm>
#include <cctype>

#include "http-parser/http_parser.h"
#include "libnodecc/net/socket.h"


int http::incoming_message::parser_on_url(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);
	self->url.append(at, length);
	return 0;
}

int http::incoming_message::parser_on_header_field(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);
	self->_partialHeaderField.append(at, length);
	return 0;
}

int http::incoming_message::parser_on_header_value(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);
	auto &field = self->_partialHeaderField;

	if (field.empty()) {
		self->_partialHeaderValue->append(at, length);
	} else {
		std::transform(field.begin(), field.end(), field.begin(), std::tolower);
		const auto iter = self->headers.emplace(std::piecewise_construct, std::forward_as_tuple(field), std::forward_as_tuple(at, length));
		field.clear();

		self->_partialHeaderValue = &iter.first->second;

		if (!iter.second) {
			self->_partialHeaderValue->append(", ");
			self->_partialHeaderValue->append(at, length);
		}
	}

	return 0;
}

int http::incoming_message::parser_on_headers_complete(http_parser *parser) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	self->http_version_major = static_cast<uint8_t>(parser->http_major);
	self->http_version_minor = static_cast<uint8_t>(parser->http_minor);
	self->method = http_method_str((http_method)parser->method);

	if (self->on_headers_complete) {
		self->on_headers_complete();
	}

	return 0;
}

int http::incoming_message::parser_on_body(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	if (self->on_body) {
		self->on_body(self->_parserBuffer->slice(at, length));
	}

	return 0;
}

int http::incoming_message::parser_on_message_complete(http_parser *parser) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	if (self->on_message_complete) {
		self->on_message_complete();
	}

	self->method.clear();
	self->url.clear();
	self->headers.clear();

	return 0;
}


http::incoming_message::incoming_message(net::socket &socket) : socket(socket) {
	static const http_parser_settings http_req_parser_settings = {
		nullptr,
		http::incoming_message::parser_on_url,
		nullptr,
		http::incoming_message::parser_on_header_field,
		http::incoming_message::parser_on_header_value,
		http::incoming_message::parser_on_headers_complete,
		http::incoming_message::parser_on_body,
		http::incoming_message::parser_on_message_complete,
	};


	this->_parser = new http_parser;
	http_parser_init(this->_parser, HTTP_REQUEST);
	this->_parser->data = this;

	// the _partialHeaderValue should always have a target
	this->_partialHeaderValue = &this->_partialHeaderField;

	this->headers.max_load_factor(0.75);

	socket.on_read = [this](int err, const util::buffer &buffer) {
		if (err) {
			if (err == UV_EOF) {
				http_parser_execute(this->_parser, &http_req_parser_settings, nullptr, 0);
			}

			this->socket.close();
		} else {
			this->_parserBuffer = &buffer;
			size_t nparsed = http_parser_execute(this->_parser, &http_req_parser_settings, buffer, buffer.size());
			this->_parserBuffer = nullptr;

			// TODO: handle upgrade
			if (this->_parser->upgrade == 1 || nparsed != buffer.size()) {
				this->socket.close();
			}
		}
	};
}

http::incoming_message::~incoming_message() {
	delete this->_parser;
}
