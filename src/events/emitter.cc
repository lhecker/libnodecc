#include "libnodecc/events/emitter.h"


namespace node {
namespace events {
namespace detail {

event_handler_root::~event_handler_root() {
	auto ptr = this->head;

	// set head to nullptr early to prevent recursive calls to clear()
	this->head = nullptr;
	this->tail = nullptr;

	while (ptr) {
		const auto tmp = ptr;
		ptr = ptr->next;
		delete tmp;
	}
}

void event_handler_root::push_back(event_handler_base* ptr) noexcept {
	if (this->tail) {
		this->tail->next = ptr;
	} else {
		this->head = ptr;
	}

	this->tail = ptr;
}

void event_handler_root::erase(iterator it) noexcept {
	const auto next = it->next;

	if (it._prev) {
		it._prev->next = next;

		if (!next) {
			this->tail = it._prev;
		}
	} else {
		// prev is null --> ptr is head
		this->head = next;

		if (!next) {
			this->tail = nullptr;
		}
	}

	delete it._curr;
}

} // namespace detail

const void* emitter::kEraseAll = (const void*)&emitter::kEraseAll;

void emitter::off(const events::detail::base_type& type, void* iter) {
	const auto& it = this->_events.find((void*)&type);

	if (it != this->_events.cend()) {
		const auto rend = it->second.end();
		auto rit = it->second.begin();

		while (rit != rend) {
			if (&*rit == iter) {
				it->second.erase(rit);

				if (it->second.head == nullptr) {
					this->_events.erase(it);
				}

				return;
			}
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
