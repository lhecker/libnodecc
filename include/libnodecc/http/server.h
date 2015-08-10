#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <list>

#include "../net/server.h"
#include "incoming_message.h"
#include "outgoing_message.h"


namespace node {
namespace http {

class server : public node::net::server {
public:
	class server_request : public node::http::incoming_message {
		friend class server;

	public:
		using incoming_message::incoming_message;
	};

	class server_response : public node::http::outgoing_message {
		friend class server;

	public:
		explicit server_response(const node::shared_ptr<node::net::socket>& socket);

		uint16_t status_code() const;
		void set_status_code(uint16_t code);

	protected:
		void _end(const node::buffer chunks[], size_t chunkcnt) override;

		void compile_headers(node::mutable_buffer& buf) override;

	private:
		uint16_t _status_code;
		bool _shutdown_on_end;
	};


	typedef std::shared_ptr<server_request> request;
	typedef std::shared_ptr<server_response> response;

	explicit server();

	void _destroy() override;

	node::callback<void(const server::request& req, const server::response& res)> request_callback;

protected:
	~server() override = default;

private:
	std::shared_ptr<bool> _is_destroyed;
	std::list<node::shared_ptr<net::socket>> _clients;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
