#ifndef nodecc_http_request_response_proto_h
#define nodecc_http_request_response_proto_h

#include <string>
#include <unordered_map>


namespace net {
	class socket;
}

namespace util {
	class buffer;
}


namespace http {

class request_response_proto {
public:
	explicit request_response_proto();

	virtual ~request_response_proto();


	const std::string &header(const std::string &key);

	/**
	 * Sets a header.
	 *
	 * If the a "transfer-encoding" header is set, it *must*
	 * either include "chunked" as the last comma-seperated entry,
	 * or a content-length entry, as per HTTP specification.
	 */
	void set_header(const std::string &key, const std::string &value);

	bool headers_sent() const;
	

	bool write(const util::buffer &buf);
	bool write(const util::buffer bufs[], size_t bufcnt);

	bool end();

protected:
	virtual void send_headers() = 0;
	virtual bool socket_write(const util::buffer bufs[], size_t bufcnt) = 0;

	std::unordered_map<std::string, std::string> _headers;
	bool _headers_sent;
	bool _is_chunked;
};

} // namespace http

#endif // nodecc_http_request_response_proto_h
