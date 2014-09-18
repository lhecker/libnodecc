#include "libnodecc/uv/handle.h"


void uv::handle_on_close(uv_handle_t* handle) {
	uv::handle<uv_handle_t>* self = reinterpret_cast<uv::handle<uv_handle_t>*>(handle->data);

	if (self && self->on_close) {
		self->on_close();
	}
}
