#ifndef nodecc_http_outgoing_message_h
#define nodecc_http_outgoing_message_h

#include <string>
#include <unordered_map>

#include "../buffer.h"
#include "../stream.h"


namespace node {

namespace net {
class socket;
}
}


namespace node {
namespace http {

class outgoing_message : public node::stream::writable<int, node::buffer> {
public:
	explicit outgoing_message(const std::shared_ptr<node::net::socket>& socket);

	const std::shared_ptr<node::net::socket>& socket();

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
	void http_write(const node::buffer bufs[], size_t bufcnt, bool end);

	void _destroy();

	void _write(const node::buffer chunks[], size_t chunkcnt) override;
	void _end(const node::buffer chunks[], size_t chunkcnt) override;

	virtual void compile_headers(node::mutable_buffer& buf) = 0;

	// needs to be directly accessed by certain subclasses
	std::unordered_map<node::hashed_buffer, node::buffer> _headers;
	std::shared_ptr<node::net::socket> _socket;
	bool _headers_sent;
	bool _is_chunked;
};

} // namespace http
} // namespace node

#endif // nodecc_http_outgoing_message_h
