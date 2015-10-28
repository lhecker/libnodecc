#include "libnodecc/events/emitter.h"


namespace node {
namespace events {

const void* emitter::kEraseAll = (const void*)&emitter::kEraseAll;

void emitter::off(const events::detail::base_type& type, void* iter) {
	const auto& it = this->_events.find((void*)&type);

	if (it != this->_events.cend()) {
		detail::event_handler_base* prev = nullptr;
		detail::event_handler_base* ptr = it->second.head;

		while (ptr) {
			if (ptr == iter) {
				if (it->second.remove(prev, ptr)) {
					this->_events.erase(it);
				}

				return;
			}

			prev = ptr;
			ptr = ptr->next;
		}
	}
}

void emitter::removeAllListeners(const events::detail::base_type& type) {
	if (this->_locked_type == (void*)&type) {
		this->_locked_type = nullptr;
	} else {
		this->_events.erase((void*)&type);
	}
}

void emitter::removeAllListeners() {
	if (this->_locked_type) {
		this->_locked_type = (void*)kEraseAll;
	} else {
		this->_events.clear();
	}
}

} // namespace events
} // namespace node
