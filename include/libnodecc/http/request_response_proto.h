#ifndef nodecc_http_request_response_proto_h
#define nodecc_http_request_response_proto_h

#include <string>
#include <unordered_map>

#include "../stream.h"


namespace node {
class buffer;
class mutable_buffer;

namespace net {
class socket;
}
}


namespace node {
namespace http {

class request_response_proto : public node::stream::writable<node::buffer> {
public:
	explicit request_response_proto();

	virtual ~request_response_proto();


	const std::string& header(const std::string& key);

	/**
	 * Sets a header.
	 *
	 * If the a "transfer-encoding" header is set, it *must*
	 * either include "chunked" as the last comma-seperated entry,
	 * or a content-length entry, as per HTTP specification.
	 */
	void set_header(const std::string& key, const std::string& value);

	bool headers_sent() const;

	inline void send_headers() {
		this->http_write(nullptr, 0, false);
	}

protected:
	void _write(const node::buffer chunks[], size_t chunkcnt) override;
	void _end(const node::buffer chunks[], size_t chunkcnt) override;

	virtual void compile_headers(node::mutable_buffer& buf) = 0;
	virtual void socket_write(const node::buffer bufs[], size_t bufcnt) = 0;

	// needs to be directly accessed by certain subclasses
	std::unordered_map<std::string, std::string> _headers;

private:
	void http_write(const node::buffer bufs[], size_t bufcnt, bool end);

	bool _headers_sent;
	bool _is_chunked;
};

} // namespace http
} // namespace node

#endif // nodecc_http_request_response_proto_h
