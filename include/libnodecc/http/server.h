#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <functional>
#include <unordered_set>

#include "../net/server.h"
#include "incoming_message.h"
#include "server_response.h"


namespace node {
namespace http {

class server : public node::net::server {
public:
	NODE_ADD_CALLBACK(public, request, void, http::incoming_message& req, http::server_response& res)

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
	req_res_pack* _clients_head;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
