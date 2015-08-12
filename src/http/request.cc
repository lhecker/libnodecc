#include "libnodecc/http/request.h"

#include <http-parser/http_parser.h>


namespace node {
namespace http {


namespace client {
namespace detail {

request::request(const node::shared_ptr<node::net::socket>& socket, const node::buffer& host, const node::buffer& method, const node::buffer& path) : outgoing_message(socket), _host(host), _method(method), _path(path) {
}

void request::compile_headers(node::mutable_buffer& buf) {
	{
		buf.append(this->_method);
		buf.push_back(' ');
		buf.append(this->_path);
		buf.append(" HTTP/1.1\r\nhost: ");
		buf.append(this->_host);
		buf.append("\r\n");
	}

	{
		for (const auto& iter : this->_headers) {
			buf.append(iter.first);
			buf.append(": ");
			buf.append(iter.second);
			buf.append("\r\n");
		}

		buf.append("\r\n");
	}
}


response::response(const node::shared_ptr<node::net::socket>& socket) : incoming_message(socket, HTTP_RESPONSE) {
}


static NODE_HTTP_REQUEST_GENERATOR_SIGNATURE {
	const auto socket = node::make_shared<node::net::socket>();
	const auto req = node::make_shared<detail::request>(socket, host, method, path);
	const auto res = node::make_shared<detail::response>(socket);

	socket->destroy_signal.connect([req, res]() {
		req->_destroy();
		res->_destroy();
	});

	socket->connect_callback.connect([req, res, cb](int err) {
		cb(err, req, res);

		if (err == 0) {
			// start reading for incoming_message
			req->socket()->resume();
		}

		req->socket()->connect_callback.clear();
	});

	socket->init(loop);

	return socket;
}

} // namespace detail
} // namespace client


void request(node::loop& loop, const node::buffer& method, const node::buffer& url, const client::on_connect_t& cb) {
	http_parser_url parser;
	const int r = http_parser_parse_url(url.data<char>(), url.size(), false, &parser);

	if (r != 0 || !(parser.field_set & (1 << UF_HOST))) {
		throw std::invalid_argument("invalid url");
	}

	const auto& host_field = parser.field_data[UF_HOST];
	const auto host = url.slice(host_field.off, host_field.off + host_field.len);

	node::buffer path;

	if (parser.field_set & (1 << UF_PATH)) {
		const auto beg = parser.field_data[UF_PATH].off;
		const auto end = parser.field_set & (1 << UF_QUERY) ? parser.field_data[UF_QUERY].off + parser.field_data[UF_QUERY].len : beg + parser.field_data[UF_PATH].len;
		path = url.slice(beg, end);
	} else {
		using namespace node::literals;
		path = "/"_view;
	}

	const auto socket = _generate(loop, host, method, path, cb);
	socket->connect(host, parser.port ? parser.port : 80);
}

void request(node::loop& loop, const sockaddr& addr, const node::buffer& host, const node::buffer& method, const node::buffer& path, const client::on_connect_t& cb) {
	const auto socket = _generate(loop, host, method, path, cb);
	socket->connect(addr);
}

} // namespace node
} // namespace http
