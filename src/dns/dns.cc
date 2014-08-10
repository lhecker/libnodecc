#include "libnodecc/dns/dns.h"

#include <uv.h>


struct getaddrinfo_packed_req {
	explicit getaddrinfo_packed_req(const dns::on_lookup_t &cb) : cb(cb) {
		this->req.data = this;
	}

	uv_getaddrinfo_t req;
	dns::on_lookup_t cb;
};


void dns::lookup(uv_loop_t *loop, const std::string &domain, const dns::on_lookup_t &cb) {
	auto packed_req = new getaddrinfo_packed_req(cb);

	uv_getaddrinfo(loop, &packed_req->req, [](uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
		auto packed_req = reinterpret_cast<getaddrinfo_packed_req*>(req->data);

		packed_req->cb(status, res);
	}, domain.c_str(), nullptr, nullptr);
}
