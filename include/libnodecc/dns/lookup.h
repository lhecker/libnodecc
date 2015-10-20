#ifndef nodecc_dns_dns_h
#define nodecc_dns_dns_h

#include "../buffer.h"
#include "../loop.h"


namespace node {
namespace dns {
namespace detail {

struct packed_req_base {
	virtual ~packed_req_base() = default;

	virtual void emit(std::error_code* err, const std::shared_ptr<addrinfo>& info) = 0;

	uv_getaddrinfo_t req;
};

template<typename T>
struct packed_req : packed_req_base {
	constexpr packed_req(T&& cb) : cb(std::forward<T>(cb)) {}

	void emit(std::error_code* err, const std::shared_ptr<addrinfo>& info) override {
		this->cb(err, info);
	}

	T cb;
};

void lookup(std::unique_ptr<packed_req_base>&& pack, node::loop& loop, const node::string& domain, const node::string& service, const addrinfo* hints = nullptr);

} // namespace detail


template<typename T>
void lookup(T&& cb, node::loop& loop, const node::string& domain, const node::string& service, const addrinfo* hints = nullptr) {
	lookup(std::unique_ptr<detail::packed_req_base>(new detail::packed_req<T>(std::forward<T>(cb))), loop, domain, service, hints);
}

template<typename T>
void lookup(T&& cb, node::loop& loop, const node::string& domain, const addrinfo* hints = nullptr) {
	lookup(std::unique_ptr<detail::packed_req_base>(new detail::packed_req<T>(std::forward<T>(cb))), loop, domain, node::string(), hints);
}

template<typename T>
void lookup(T&& cb, node::loop& loop, const node::string& domain, uint16_t port, const addrinfo* hints = nullptr) {
	lookup(std::unique_ptr<detail::packed_req_base>(new detail::packed_req<T>(std::forward<T>(cb))), loop, domain, node::to_string(port), hints);
}

} // namespace dns
} // namespace node

#endif // nodecc_dns_dns_h
