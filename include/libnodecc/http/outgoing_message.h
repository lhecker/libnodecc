#ifndef nodecc_http_outgoing_message_h
#define nodecc_http_outgoing_message_h

#include <string>
#include <unordered_map>

#include "../buffer.h"
#include "../tcp/socket.h"
#include "../stream.h"


namespace node {
namespace http {

class outgoing_message : public node::object, public node::stream::writable<outgoing_message, node::buffer> {
	friend class server;

public:
	explicit outgoing_message(const node::shared_ptr<node::tcp::socket>& socket);

	const node::shared_ptr<node::tcp::socket>& socket();

	const node::buffer header(node::hashed_buffer& key);

	/**
	 * Sets a header.
	 *
	 * If the a "transfer-encoding" header is set, it *must*
	 * either include "chunked" as the last comma-seperated entry,
	 * or a content-length entry, as per HTTP specification.
	 */
	void set_header(node::hashed_buffer key, node::buffer value);

	bool headers_sent() const;

	inline void send_headers() {
		this->write(nullptr, 0);
	}

	void destroy();

protected:
	~outgoing_message() override = default;

	void http_write(const node::buffer bufs[], size_t bufcnt, bool end);

	void _destroy() override;

	void _write(const node::buffer chunks[], size_t chunkcnt) override;
	void _end(const node::buffer chunks[], size_t chunkcnt) override;

	virtual void compile_headers(node::mutable_buffer& buf) = 0;

	// needs to be directly accessed by certain subclasses
	std::unordered_map<node::hashed_buffer, node::buffer> _headers;
	node::shared_ptr<node::tcp::socket> _socket;
	bool _headers_sent;
	bool _is_chunked;
};

} // namespace http
} // namespace node

#endif // nodecc_http_outgoing_message_h
