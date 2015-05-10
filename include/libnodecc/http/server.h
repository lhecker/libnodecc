#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <functional>

#include "../net/server.h"
#include "incoming_message.h"
#include "server_response.h"


namespace node {
namespace http {

class server : public node::net::server {
public:
	typedef std::shared_ptr<node::http::incoming_message> request;
	typedef std::shared_ptr<node::http::server_response> response;

	NODE_CALLBACK_ADD(public, request, void, const node::http::server::request& req, const node::http::server::response& res)

public:
	explicit server();

	void close();

	template<typename F>
	void close(F f) {
		this->on_close(std::forward<F>(f));
		this->close();
	}

private:
	class req_res_pack;
	std::weak_ptr<req_res_pack> _clients_head;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
