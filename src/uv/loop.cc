#include "libnodecc/uv/loop.h"

#include <type_traits>


uv::loop::loop() noexcept {
	uv_loop_init(&this->_loop);
}

uv::loop::~loop() noexcept {
	uv_loop_close(&this->_loop);
}

void uv::loop::run() {
	uv_run(&this->_loop, UV_RUN_DEFAULT);
}

bool uv::loop::run_once() {
	return 0 != uv_run(&this->_loop, UV_RUN_ONCE);
}

bool uv::loop::run_nowait() {
	return 0 != uv_run(&this->_loop, UV_RUN_NOWAIT);
}

uv::loop::operator uv_loop_t*() {
	return &this->_loop;
}

uv::loop::operator const uv_loop_t*() const {
	return &this->_loop;
}

bool uv::loop::alive() {
	return uv_loop_alive(&this->_loop);
}
