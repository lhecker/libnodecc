#include "libnodecc/loop.h"

#include <system_error>


namespace node {

loop::loop() noexcept {
	// always returns 0
	uv_loop_init(&this->_loop);

	// always returns 0
	uv_async_init(&this->_loop, &this->_tick_async, &loop::_on_async);

	uv_unref(reinterpret_cast<uv_handle_t*>(&this->_tick_async));

	this->_loop.data = this;
	this->_tick_async.data = this;
}

loop::~loop() noexcept {
	uv_close(reinterpret_cast<uv_handle_t*>(&this->_tick_async), nullptr);
	this->run(); // close the async handle
	uv_loop_close(&this->_loop);
}

bool loop::run() {
	return 0 != uv_run(&this->_loop, UV_RUN_DEFAULT);
}

bool loop::run_once() {
	return 0 != uv_run(&this->_loop, UV_RUN_ONCE);
}

bool loop::run_nowait() {
	return 0 != uv_run(&this->_loop, UV_RUN_NOWAIT);
}

void loop::stop() {
	uv_stop(&this->_loop);
}

bool loop::alive() {
	return uv_loop_alive(&this->_loop) != 0;
}

loop::operator uv_loop_t*() {
	return &this->_loop;
}

loop::operator const uv_loop_t*() const {
	return &this->_loop;
}

void loop::_on_async(uv_async_t* handle) noexcept {
	auto self = reinterpret_cast<loop*>(handle->data);

	for (const auto& cb : self->_tick_callbacks) {
		cb();

		if (self->_loop.stop_flag) {
			return;
		}
	}

	/*
	 * We do the shrink first and the clear, because:
	 * The vector for the next loop iteration will be empty (all elements are deconstructed),
	 * BUT it will be always have a capacity at least as large as the size of the previous iteration.
	 *
	 * TODO: Using a the a formulae similiar to "average_size_over_last_N_iterations * a_factor_greater_one"
	 * should perform better if the amount of callbacks fluctuates heavily  than the current implementation.
	 */
	self->_tick_callbacks.shrink_to_fit();
	self->_tick_callbacks.clear();
}

} // namespace node
