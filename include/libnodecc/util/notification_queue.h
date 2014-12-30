#ifndef nodecc_util_notification_queue_h
#define nodecc_util_notification_queue_h

#include <atomic>
#include <mutex>
#include <vector>

#include "../uv/async.h"


namespace node {
namespace util {

template<typename T>
class notification_queue : private node::uv::async {
public:
	/*
	 * This callback will be called once for each notification,
	 * that has been passed to the queue.
	 */
	NODE_ADD_CALLBACK(public, notification, void, const T&)

	/*
	 * Passes a reference to all notifications.
	 * 
	 * This interface might change, when the underlying
	 * data structure changes - consider using the notification callback.
	 */
	NODE_ADD_CALLBACK(public, notifications, void, const std::vector<T>&)

public:
	explicit notification_queue() : node::uv::async() {}

	bool init(node::loop& loop) {
		bool ok = node::uv::async::init(loop, [](uv_async_t* handle) {
			auto self = reinterpret_cast<util::notification_queue<T>*>(handle->data);

			std::vector<T> queue;

			{
				std::lock_guard<std::mutex> lock(self->_mutex);
				std::swap(queue, self->_queue);
			}

			self->emit_notifications_s(queue);

			for (const T& v : queue) {
				if (!self->emit_notification_s(v)) {
					break;
				}
			}
		});

		if (ok) {
			bool is_empty;
			{
				std::lock_guard<std::mutex> lock(this->_mutex);
				is_empty = this->_queue.empty();
			}

			if (!is_empty) {
				this->send();
			}
		}

		return ok;
	}

	void push_back(const T& value) {
		{
			std::lock_guard<std::mutex> lock(this->_mutex);
			this->_queue.push_back(value);
		}

		this->send();
	}

	void push_back(T&& value) {
		{
			std::lock_guard<std::mutex> lock(this->_mutex);
			this->_queue.push_back(std::forward<T>(value));
		}

		this->send();
	}

	template<typename... Args>
	void emplace(Args&&... args) {
		{
			std::lock_guard<std::mutex> lock(this->_mutex);
			this->_queue.emplace_back(std::forward<Args>(args)...);
		}

		this->send();
	}

	template<typename... Args>
	void close(Args&&... args) {
		this->_on_notification = nullptr;
		node::uv::handle<uv_async_t>::close(std::forward<Args>(args)...);
	}

private:
	std::vector<T> _queue;
	std::mutex _mutex;
};

} // namespace util
} // namespace node

#endif // nodecc_util_notification_queue_h
