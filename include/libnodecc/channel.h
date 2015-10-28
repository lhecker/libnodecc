#ifndef nodecc_channel_h
#define nodecc_channel_h

#include <mutex>
#include <vector>


namespace node {

template<typename T>
class channel {
public:
	typedef std::vector<T> queue_type;

	void send(const T& value) {
		std::lock_guard<std::mutex> lock(this->_mutex);
		this->_q.push_back(value);
	}

	void send(T&& value) {
		std::lock_guard<std::mutex> lock(this->_mutex);
		this->_q.push_back(std::forward<T>(value));
	}

	template<typename... Args>
	void send(Args&&... args) {
		std::lock_guard<std::mutex> lock(this->_mutex);
		this->_q.emplace_back(std::forward<Args>(args)...);
	}

	queue_type recv() {
		queue_type q;

		{
			std::lock_guard<std::mutex> lock(this->_mutex);

			// reserve some space for the next channel round to reduce reallocations
			const auto capacity = this->_q.capacity();
			q.reserve(capacity + (capacity >> 1));

			std::swap(q, this->_q);
		}

		return q;
	}

private:
	std::vector<T> _q;
	std::mutex _mutex;
};

} // namespace node

#endif // nodecc_channel_h
