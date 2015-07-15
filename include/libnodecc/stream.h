#ifndef nodecc_stream_h
#define nodecc_stream_h

#include "event.h"


namespace node {
namespace stream {

template <typename T, typename E>
class writable;


namespace detail {

template<typename T, typename E>
class base {
public:
	node::event<void(E err)> on_error;
};

} // namespace detail


template<typename T, typename E>
class readable : public virtual detail::base<T, E> {
public:
	friend class writable<T, E>;

	virtual void resume() = 0;
	virtual void pause() = 0;

	void pipe(const node::stream::writable<T, E>& target, bool end = true) {
		this->on_data([this, &target](const T chunks[], size_t chunkcnt) {
			if (!target.write(chunks, chunkcnt)) {
				this->pause();
			}
		});

		target->on_drain([this]() {
			this->resume();
		});

		if (end) {
			this->on_end([&target]() {
				target.end();
			});
		}
	}

	node::event<void(const T chunks[], size_t chunkcnt)> on_data;
	node::event<void()> on_end;
};

template<typename T, typename E>
class writable : public virtual detail::base<T, E> {
public:
	explicit writable(size_t hwm = 16 * 1024, size_t lwm = 4 * 1024) : _hwm(hwm), _lwm(lwm), _wm(0), _was_flooded(false) {}

	/*
	* TODO: add callbacks to write() and writev()
	* BUT:
	* A developer might write something like this:
	*   void cb() { writer.write(buffer, cb); }
	*   cb();
	* which might lead to infinite recursion, if the write() function calls the cb synchronously.
	* (This is for instance the case for node::stream<T> which uses the synchronous uv_try_write.)
	*/

	inline size_t highwatermark() const {
		return this->_hwm;
	}

	inline void set_highwatermark(size_t hwm) {
		this->_hwm = hwm;
	}

	inline size_t lowwatermark() const {
		return this->_lwm;
	}

	inline void set_lowwatermark(size_t lwm) {
		this->_lwm = lwm;
	}

	inline bool write(const T& chunk) {
		return this->write(&chunk, 1);
	}

	bool write(const T chunks[], size_t chunkcnt) {
		this->_write(chunks, chunkcnt);
		this->_was_flooded = this->_wm >= this->_hwm;
		return this->_was_flooded;
	}

	inline bool end() {
		this->_end(nullptr, 0);
		return this->_was_flooded;
	}

	inline bool end(const T& chunk) {
		return this->end(&chunk, 1);
	}

	bool end(const T chunks[], size_t chunkcnt) {
		this->_end(chunks, chunkcnt);
		this->_was_flooded = this->_wm >= this->_hwm;
		return this->_was_flooded;
	}

	node::event<void()> on_drain;

protected:
	inline void increase_watermark(size_t n) {
		this->_hwm += n;
	}

	void decrease_watermark(size_t n) {
		this->_wm = this->_wm > n ? this->_wm - n : 0;

		if (this->_was_flooded && this->_wm <= this->_lwm) {
			this->on_drain.emit();
			this->_was_flooded = false;
		}
	}

	virtual void _write(const T chunks[], size_t chunkcnt) = 0;
	virtual void _end(const T chunks[], size_t chunkcnt) = 0;

private:
	size_t _hwm;
	size_t _lwm;
	size_t _wm;
	bool _was_flooded;
};

template<typename T, typename E>
class duplex : public readable<T, E>, public writable<T, E> {
};

} // namespace stream
} // namespace node

#endif // nodecc_stream_h
