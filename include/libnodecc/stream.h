#ifndef nodecc_stream_h
#define nodecc_stream_h

#include "common.h"


namespace node {
namespace stream {

template <typename T>
class writable;

template<typename T, typename E>
class readable {
	friend class writable<T>;

	NODE_ADD_CALLBACK(public, data, void, E err, const T chunks[], size_t chunkcnt);
	NODE_ADD_CALLBACK(public, end, void);

public:
	virtual void resume() = 0;
	virtual void pause() = 0;

	void pipe(const node::stream::writable<T>& target, bool end = true) {
		this->on_data([this, target](E err, const T chunks[], size_t chunkcnt) {
			if (err || !target.writev(chunks, chunkcnt)) {
				this->pause();
			}
		});

		target->on_drain([this]() {
			this->resume();
		});

		if (end) {
			this->on_end([target]() {
				target.end();
			});
		}
	}
};

template<typename T>
class writable {
	NODE_ADD_CALLBACK(public, drain, void);

public:
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

	bool write(const T& chunk) {
		this->writev(&chunk, 1);
	};

	bool writev(const T chunks[], size_t chunkcnt) {
		this->_write(chunks, chunkcnt);
		this->_was_flooded = this->_wm >= this->_hwm;
		return this->_was_flooded;
	}

	virtual void _write(const T chunks[], size_t chunkcnt) = 0;
	virtual void end() = 0;

protected:
	inline void increase_watermark(size_t n) {
		this->_hwm += n;
	}

	void decrease_watermark(size_t n) {
		this->_wm = this->_wm > n ? this->_wm - n : 0;

		// TODO: optimize the point in time at which the writer emits the drain event
		if (this->_was_flooded && this->_wm < this->_lwm) {
			this->emit_drain_s();
			this->_was_flooded = false;
		}
	}

	bool writable_return_value() {
	}

private:
	size_t _hwm = 16 * 1024;
	size_t _lwm =  4 * 1024;
	size_t _wm = 0;
	bool _was_flooded = false;
};

template<typename T, typename E>
class duplex : public readable<T, E>, public writable<T> {
};

} // namespace stream
} // namespace node

#endif // nodecc_stream_h
