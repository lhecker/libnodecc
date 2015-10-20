#include "libnodecc/util/timer.h"


namespace node {
namespace util {

timer::timer(node::loop& loop) : uv::handle<uv_timer_t>() {
	node::uv::check(uv_timer_init(loop, *this));
}

uint64_t timer::repeat() const {
	return uv_timer_get_repeat(*this);
}

void timer::set_repeat(uint64_t repeat) {
	uv_timer_set_repeat(*this, repeat);
}

void timer::again() {
	node::uv::check(uv_timer_again(*this));
}

void timer::start(uint64_t timeout, uint64_t repeat) {
	node::uv::check(uv_timer_start(*this, [](uv_timer_t* timer) {
		auto self = reinterpret_cast<node::util::timer*>(timer->data);
		self->timeout_callback.emit();
	}, timeout, repeat));
}

void timer::stop() {
	node::uv::check(uv_timer_stop(*this));
}

} // namespace node
} // namespace util
