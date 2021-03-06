#ifndef nodecc_http_server_h
#define nodecc_http_server_h

#include <list>

#include "../tcp/server.h"
#include "incoming_message.h"
#include "outgoing_message.h"


namespace node {
namespace http {

class server : public node::tcp::server {
public:
	class server_request : public node::http::incoming_message {
		friend class server;

	public:
		using incoming_message::incoming_message;
	};

	class server_response : public node::http::outgoing_message {
		friend class server;

	public:
		explicit server_response(const node::shared_ptr<node::tcp::socket>& socket);

		uint16_t status_code() const;
		void set_status_code(uint16_t code);

	protected:
		void _end(const node::buffer chunks[], size_t chunkcnt) override;

		void compile_headers(node::mutable_buffer& buf) override;

	private:
		uint16_t _status_code;
		bool _shutdown_on_end;
	};


	typedef node::shared_ptr<server_request> request;
	typedef node::shared_ptr<server_response> response;

	static const node::events::symbol<void(const node::http::server::request& req, const node::http::server::response& res)> request_event;


	explicit server(node::loop& loop);

protected:
	~server() override = default;

	void _destroy() override;

private:
	std::shared_ptr<bool> _is_destroyed;
	std::list<node::shared_ptr<tcp::socket>> _clients;
};

} // namespace http
} // namespace node

#endif // nodecc_http_server_h
