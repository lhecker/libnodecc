#include "libnodecc/http/incoming_message.h"

#include <algorithm>
#include <cctype>

#include "http-parser/http_parser.h"
#include "libnodecc/net/socket.h"


static int on_url(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);
	self->url.append(at, length);
	return 0;
}

static int on_header_field(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	self->_partialHeaderField.append(at, length);

	return 0;
}

static int on_header_value(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);
	auto &field = self->_partialHeaderField;

	if (field.empty()) {
		self->_partialHeaderValue->append(at, length);
	} else {
		std::transform(field.begin(), field.end(), field.begin(), std::tolower);
		const auto iter = self->headers.emplace(std::piecewise_construct, std::forward_as_tuple(field), std::forward_as_tuple(at, length));
		field.clear();

		self->_partialHeaderValue = &iter.first->second;
	}

	return 0;
}

static int on_headers_complete(http_parser *parser) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	self->http_version_major = static_cast<uint8_t>(parser->http_major);
	self->http_version_minor = static_cast<uint8_t>(parser->http_minor);
	self->method = http_method_str((http_method)parser->method);

	if (self->on_headers_complete) {
		self->on_headers_complete();
	}

	return 0;
}

static int on_body(http_parser *parser, const char *at, size_t length) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	if (self->on_body) {
		uv_buf_t buf;
		buf.base = (char *)at;
		buf.len = length;
		self->on_body(buf);
	}

	return 0;
}

static int on_message_complete(http_parser *parser) {
	auto self = static_cast<http::incoming_message *>(parser->data);

	if (self->on_message_complete) {
		self->on_message_complete();
	}

	self->method.clear();
	self->url.clear();
	self->headers.clear();

	return 0;
}


static const http_parser_settings http_req_parser_settings = {
	nullptr,
	on_url,
	nullptr,
	on_header_field,
	on_header_value,
	on_headers_complete,
	on_body,
	on_message_complete,
};


http::incoming_message::incoming_message(net::socket &socket) : socket(socket) {
	this->_parser = new http_parser;
	http_parser_init(this->_parser, HTTP_REQUEST);
	this->_parser->data = this;

	this->_partialHeaderValue = &this->_partialHeaderField; // always a target for pointers

	this->headers.max_load_factor(0.75);

	socket.on_alloc = [](size_t suggested_size, uv_buf_t *buf) {
		buf->len = suggested_size;
		buf->base = new char[suggested_size];
	};

	socket.on_read = [this](ssize_t nread, const uv_buf_t *buf) {
		if (nread < 0) {
			if (nread == UV_EOF) {
				http_parser_execute(this->_parser, &http_req_parser_settings, nullptr, 0);
			}

			this->socket.close();
		} else if (nread > 0) {
			ssize_t nparsed = http_parser_execute(this->_parser, &http_req_parser_settings, buf->base, nread);

			// TODO: handle upgrade
			if (this->_parser->upgrade == 1 || nparsed != nread) {
				this->socket.close();
			}
		}

		if (buf->base) {
			delete[] buf->base;
		}
	};
}
