#ifndef nodecc_http_client_request_h
#define nodecc_http_client_request_h

#include <unordered_map>

#include "../net/socket.h"


namespace http {

class client_request : net::socket {
public:
	typedef std::function<void()> on_response_t;


	explicit client_request();

	using net::socket::init;

	/*
	* If the a transfer-encoding header is set, it *must*
	* either include "chunked" as the last comma-seperated entry,
	* or a content-length entry.
	*/

	const std::string &getHeader(const std::string &key);
	void setHeader(const std::string &key, const std::string &value);
	bool headersSent() const;

	void write(const std::string &str);
	void end();


	std::string method;
	std::string path;
	std::string hostname;


	using net::socket::on_read;
	on_response_t on_response;

private:
	void sendHeaders();

	std::unordered_map<std::string, std::string> _headers;
	bool _headersSent;
	bool _isChunked;
};

} // namespace http

#endif // nodecc_http_client_request_h
