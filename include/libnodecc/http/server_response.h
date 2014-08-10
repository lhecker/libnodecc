#ifndef nodecc_http_server_response_h
#define nodecc_http_server_response_h

#include <string>
#include <unordered_map>


namespace net {
class socket;
}


namespace http {

class server_response {
public:
	explicit server_response(net::socket &socket);

	/*
	 * If the a transfer-encoding header is set, it *must*
	 * either include "chunked" as the last comma-seperated entry,
	 * or a content-length entry.
	 */
	template <typename T>
	const std::string &getHeader(T&& key) { return this->_headers[std::forward<T>(key)]; }
	template <typename... T>
	void setHeader(T&&... args) { this->_headers.emplace(std::forward<T>(args)...); }

	inline bool headersSent() const { return this->_headersSent; }

	void write(const std::string &str);
	void end();


	net::socket &socket;

	uint16_t statusCode;

private:
	void sendHeaders();

	std::unordered_map<std::string, std::string> _headers;
	bool _headersSent;
	bool _isChunked;
};

} // namespace http

#endif // nodecc_http_server_response_h
