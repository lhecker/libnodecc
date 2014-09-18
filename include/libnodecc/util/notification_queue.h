#ifndef nodecc_util_notification_queue_h
#define nodecc_util_notification_queue_h

#include <atomic>
#include <mutex>
#include <vector>

#include "../uv/async.h"


namespace util {

template<typename T>
class notification_queue : public uv::async {
public:
	typedef std::function<void(const T&)> on_notification_t;


	explicit notification_queue() : uv::async() {}

	bool init(uv::loop& loop) {
		bool ok = uv::async::init(loop, [](uv_async_t* handle) {
			auto self = reinterpret_cast<util::notification_queue<T>*>(handle->data);

			std::vector<T> queue;

			{
				std::lock_guard<std::mutex> lock(self->_mutex);
				std::swap(queue, self->_queue);
			}

			for (const T& v : queue) {
				if (self->on_notification) {
					self->on_notification(v);
				} else {
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
		this->on_notification = nullptr;
		uv::handle<uv_async_t>::close(std::forward<Args>(args)...);
	}


	on_notification_t on_notification;

private:
	std::vector<T> _queue;
	std::mutex _mutex;
};

} // namespace util

#endif // nodecc_util_notification_queue_h
