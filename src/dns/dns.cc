#include "libnodecc/dns/dns.h"

#include <uv.h>
#include <memory>


namespace {

struct getaddrinfo_packed_req {
	explicit getaddrinfo_packed_req(const dns::on_lookup_t &cb) {
		this->req.data = this;
		this->cb = cb;
	}

	uv_getaddrinfo_t req;
	dns::on_lookup_t cb;
};

struct addrinfo_deleter {
    void operator()(addrinfo *ptr) const {
        uv_freeaddrinfo(ptr);
    }
};

};


void dns::lookup(uv_loop_t *loop, const std::string &domain, const on_lookup_t &cb) {
	auto packed_req = new getaddrinfo_packed_req(cb);

	uv_getaddrinfo(loop, &packed_req->req, [](uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
		auto packed_req = reinterpret_cast<getaddrinfo_packed_req*>(req->data);

		if (status == 0) {
			packed_req->cb(std::shared_ptr<addrinfo>(res, addrinfo_deleter()));
		} else {
			packed_req->cb(std::shared_ptr<addrinfo>());
		}
	}, domain.c_str(), nullptr, nullptr);
}
