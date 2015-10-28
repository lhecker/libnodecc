#ifndef nodecc_util_async_channel_h
#define nodecc_util_async_channel_h

#include "../channel.h"
#include "../events.h"
#include "../uv/async.h"


namespace node {
namespace util {

template<typename T>
class async_channel : public node::events::emitter {
public:
	typedef typename node::channel<T>::queue_type queue_type;

	static const node::events::type<void(const queue_type& queue)> recv_event;
	static const node::events::type<void(const std::error_code& err)> error_event;


	void send(const T& value) {
		this->_channel.send(value);

		if (this->_async) {
			this->_async->send();
		}
	}

	void send(T&& value) {
		this->_channel.send(std::forward<T>(value));

		if (this->_async) {
			this->_async->send();
		}
	}

	template<typename... Args>
	void send(Args&&... args) {
		this->_channel.send(std::forward<Args>(args)...);

		if (this->_async) {
			this->_async->send();
		}
	}

	void init(node::loop& loop) {
		if (this->_async) {
			throw std::logic_error("already initialized");
		}

		this->_async = node::make_shared<node::uv::async>(loop);

		this->_async->on(node::uv::async::error_event, [this](const std::error_code& err) {
			this->emit(error_event, err);
			this->destroy();
		});

		this->_async->on(node::uv::async::async_event, [this]() {
			const auto queue = this->_channel.recv();

			if (!queue.empty()) {
				this->emit(recv_event, queue);
			}
		});

		this->_async->send();
	}

	void destroy() {
		this->_async = nullptr;
		this->removeAllListeners();
	}

private:
	node::channel<T> _channel;
	node::shared_ptr<node::uv::async> _async;
};

template<typename T>
const node::events::type<void(const typename async_channel<T>::queue_type& queue)> async_channel<T>::recv_event;

template<typename T>
const node::events::type<void(const std::error_code& err)> async_channel<T>::error_event;

} // namespace util
} // namespace node

#endif // nodecc_util_async_channel_h
