#include "libnodecc/util/timer.h"


util::timer::timer() : uv::handle<uv_timer_t>() {
}

bool util::timer::init(uv::loop& loop) {
	return 0 == uv_timer_init(loop, *this);
}

uint64_t util::timer::repeat() const {
	return uv_timer_get_repeat(*this);
}

void util::timer::set_repeat(uint64_t repeat) {
	return uv_timer_set_repeat(*this, repeat);
}

bool util::timer::again() {
	return 0 == uv_timer_again(*this);
}

bool util::timer::start(uint64_t timeout, uint64_t repeat) {
	return 0 == uv_timer_start(*this, [](uv_timer_t* timer) {
		auto self = reinterpret_cast<util::timer*>(timer->data);

		if (self->on_timeout) {
			self->on_timeout();
		}
	}, timeout, repeat);
}

bool util::timer::start(uint64_t timeout, uint64_t repeat, on_timeout_t cb) {
	this->on_timeout = cb;
	return this->start(timeout, repeat);
}

bool util::timer::stop() {
	return 0 == uv_timer_stop(*this);
}
