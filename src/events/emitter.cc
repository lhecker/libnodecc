#include "libnodecc/events/emitter.h"


namespace node {
namespace events {
namespace detail {

const std::size_t event_handler_base::delete_flag;


event_handler_root::~event_handler_root() {
	auto ptr = this->head;

	// set head to nullptr early to prevent recursive calls to clear()
	this->head = nullptr;
	this->tail = nullptr;

	while (ptr) {
		const auto tmp = ptr;
		ptr = ptr->next;

		if (tmp->ref_count == 0) {
			delete tmp;
		} else {
			// prevent emit() from advancing
			tmp->next = nullptr;
		}
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

	if (it->ref_count == 0) {
		delete it._curr;
	} else {
		// prevent emit() from advancing
		it->next = nullptr;
	}
}

} // namespace detail


const void* emitter::kEraseAll = (const void*)&emitter::kEraseAll;

void emitter::off(const events::detail::base_symbol& symbol, void* iter) {
	const auto& it = this->_events.find((void*)&symbol);

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

			++rit;
		}
	}
}

} // namespace events
} // namespace node
