#ifndef node_cpp_http_client_request_h
#define node_cpp_http_client_request_h

#include <functional>
#include <unordered_map>
#include "../net/socket.h"


namespace http {
	class client_request : net::socket {
	public:
		typedef std::function<void()> on_response_t;


		explicit client_request(uv_loop_t *loop);

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


		std::string hostname;
		std::string path;
		std::string method;


		using net::socket::on_read;
		on_response_t on_response;

	private:
		void sendHeaders();

		std::unordered_map<std::string, std::string> _headers;
		bool _headersSent;
		bool _isChunked;
	};
}

#endif
