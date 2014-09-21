#include "libnodecc/dns/lookup.h"

#include "libnodecc/loop.h"


namespace {

struct getaddrinfo_packed_req {
	explicit getaddrinfo_packed_req(const node::dns::on_lookup_t& cb) {
		this->req.data = this;
		this->cb = cb;
	}

	uv_getaddrinfo_t req;
	node::dns::on_lookup_t cb;
};

struct addrinfo_deleter {
	void operator()(addrinfo* ptr) const {
		uv_freeaddrinfo(ptr);
	}
};

}


namespace node {
namespace dns {

void lookup(const on_lookup_t& cb, node::loop& loop, const std::string& domain, const addrinfo* hints) {
	lookup(cb, loop, domain, std::string(), hints);
}

void lookup(const on_lookup_t& cb, node::loop& loop, const std::string& domain, uint16_t port, const addrinfo* hints) {
	lookup(cb, loop, domain, std::to_string(port), hints);
}

void lookup(const on_lookup_t& cb, node::loop& loop, const std::string& domain, const std::string& service, const addrinfo* hints) {
	auto packed_req = new getaddrinfo_packed_req(cb);

	if (!hints) {
		static const addrinfo static_hints {
			0,           // ai_flags
			AF_UNSPEC,   // ai_family
			SOCK_STREAM, // ai_socktype
			0,           // ai_protocol
			0,           // ai_addrlen
			nullptr,     // ai_canonname
			nullptr,     // ai_addr
			nullptr,     // ai_next
		};

		hints = &static_hints;
	}

	uv_getaddrinfo(loop, &packed_req->req, [](uv_getaddrinfo_t* req, int status, addrinfo* res) {
		auto packed_req = reinterpret_cast<getaddrinfo_packed_req*>(req->data);

		if (status == 0) {
			packed_req->cb(std::shared_ptr<addrinfo>(res, addrinfo_deleter()));
		} else {
			packed_req->cb(std::shared_ptr<addrinfo>());
		}
	}, domain.c_str(), service.empty() ? nullptr : service.c_str(), hints);
}

} } // namespace node::dns
