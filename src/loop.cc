#include "libnodecc/loop.h"


namespace node {

loop::loop() noexcept {
	uv_loop_init(&this->_loop);
	uv_async_init(&this->_loop, &this->_tick_async, [](uv_async_t* handle) {
		auto self = reinterpret_cast<loop*>(handle->data);

		std::vector<on_tick_t> callbacks;
		std::swap(callbacks, self->_tick_callbacks);

		for (const on_tick_t& cb : callbacks) {
			cb();

			if (self->_loop.stop_flag) {
				return;
			}
		}
	});

	uv_unref(reinterpret_cast<uv_handle_t*>(&this->_tick_async));

	this->_loop.data = this;
	this->_tick_async.data = this;
	this->_tick_callbacks.reserve(8);
}

loop::~loop() noexcept {
	uv_close(reinterpret_cast<uv_handle_t*>(&this->_tick_async), nullptr);
	this->run(); // close the async handle
	uv_loop_close(&this->_loop);
}

void loop::run() {
	uv_run(&this->_loop, UV_RUN_DEFAULT);
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
	return uv_loop_alive(&this->_loop);
}

loop::operator uv_loop_t*() {
	return &this->_loop;
}

loop::operator const uv_loop_t*() const {
	return &this->_loop;
}

} // namespace node
