#include "libnodecc/dns/lookup.h"


namespace {

struct addrinfo_deleter {
	void operator()(addrinfo* ptr) const {
		uv_freeaddrinfo(ptr);
	}
};

}


namespace node {
namespace dns {
namespace detail {

void lookup(std::unique_ptr<packed_req_base>&& pack, node::loop& loop, const node::string& domain, const node::string& service, const addrinfo* hints) {
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

	pack->req.data = pack.get();

	node::uv::check(uv_getaddrinfo(loop, &pack->req, [](uv_getaddrinfo_t* req, int status, addrinfo* res) {
		auto pack = reinterpret_cast<packed_req_base*>(req->data);
		std::error_code err(status, std::system_category());
		std::shared_ptr<addrinfo> info;

		if (status == 0) {
			info.reset(res, addrinfo_deleter());
		}

		pack->emit(status == 0 ? nullptr : &err, info);

		delete pack;
	}, domain.c_str(), service.empty() ? nullptr : service.c_str(), hints));

	pack.release();
}

} // namespace detail
} // namespace dns
} // namespace node
