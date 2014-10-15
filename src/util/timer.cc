#include "libnodecc/util/timer.h"


namespace node {
namespace util {

timer::timer() : uv::handle<uv_timer_t>() {
}

bool timer::init(node::loop& loop) {
	return 0 == uv_timer_init(loop, *this);
}

uint64_t timer::repeat() const {
	return uv_timer_get_repeat(*this);
}

void timer::set_repeat(uint64_t repeat) {
	return uv_timer_set_repeat(*this, repeat);
}

bool timer::again() {
	return 0 == uv_timer_again(*this);
}

bool timer::start(uint64_t timeout, uint64_t repeat) {
	return 0 == uv_timer_start(*this, [](uv_timer_t* timer) {
		auto self = reinterpret_cast<node::util::timer*>(timer->data);
		self->emit_timeout_s();
	}, timeout, repeat);
}

bool timer::start(uint64_t timeout, uint64_t repeat, on_timeout_t cb) {
	this->_on_timeout = std::move(cb);
	return this->start(timeout, repeat);
}

bool timer::stop() {
	return 0 == uv_timer_stop(*this);
}

} // namespace node
} // namespace util
