#ifndef nodecc_channel_h
#define nodecc_channel_h

#include <atomic>
#include <mutex>
#include <vector>

#include "uv/async.h"


namespace node {

template<typename T>
class channel : private node::uv::async {
public:
	typedef std::vector<T> queue;

	/*
	 * This callback will be called once for each notification,
	 * that has been passed to the queue.
	 */
	NODE_CALLBACK_ADD(public, notifications, void, const queue&)

public:
	explicit channel() : node::uv::async() {}

	bool init(node::loop& loop) {
		const bool ok = node::uv::async::init(loop, [](uv_async_t* handle) {
			auto self = reinterpret_cast<channel<T>*>(handle->data);

			queue q;

			{
				std::lock_guard<std::mutex> lock(self->_mutex);
				std::swap(q, self->_q);
			}

			self->emit_notifications_s(q);
		});

		if (ok) {
			bool is_empty;
			{
				std::lock_guard<std::mutex> lock(this->_mutex);
				is_empty = this->_q.empty();
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
			this->_q.push_back(value);
		}

		this->send();
	}

	void push_back(T&& value) {
		{
			std::lock_guard<std::mutex> lock(this->_mutex);
			this->_q.push_back(std::forward<T>(value));
		}

		this->send();
	}

	template<typename... Args>
	void emplace(Args&&... args) {
		{
			std::lock_guard<std::mutex> lock(this->_mutex);
			this->_q.emplace_back(std::forward<Args>(args)...);
		}

		this->send();
	}

	template<typename... Args>
	void close(Args&&... args) {
		this->_on_notifications = nullptr;
		node::uv::handle<uv_async_t>::close(std::forward<Args>(args)...);
	}

private:
	std::vector<T> _q;
	std::mutex _mutex;
};

} // namespace node

#endif // nodecc_channel_h
