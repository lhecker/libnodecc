#ifndef nodecc_stream_h
#define nodecc_stream_h

#include "common.h"


namespace node {
namespace stream {

template <typename T>
class writeable;

template<typename T>
class readable {
	friend class writeable<T>;

	NODE_ADD_CALLBACK(public, data, void, const T* chunks, size_t chunkcnt);
	NODE_ADD_CALLBACK(public, end, void);

public:
	virtual void resume() = 0;
	virtual void pause() = 0;

	void pipe(const node::stream::writeable<T>& target, bool end = true) {
		this->on_data([this, target](const T* chunks, size_t chunkcnt) {
			if (!target.writev(chunks, chunkcnt)) {
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
class writeable {
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

	virtual void end() = 0;
	virtual bool write(const T& chunk) = 0;
	virtual bool writev(const T* chunks, size_t chunkcnt) {
		bool ret = true;

		for (size_t i = 0; i < chunkcnt; i++) {
			ret = this->write(chunks[i]) && ret;
		}

		return ret;
	}

protected:
	inline void increase_watermark(size_t n) {
		this->_hwm += n;
	}

	void decrease_watermark(size_t n) {
		this->_wm = this->_wm > n ? this->_wm - n : 0;

		// TODO: optimize the point when exactly the writer emits the drain event
		if (this->_state == 1 && this->_wm < this->_lwm) {
			this->emit_drain_s();
			this->_state = 0;
		}
	}

	bool writeable_return_value() {
		bool ret = this->_wm >= this->_hwm;

		if (ret) {
			this->_state = 1;
		}

		return ret;
	}

private:
	size_t _hwm = 16 * 1024;
	size_t _lwm =  4 * 1024;
	size_t _wm = 0;
	uint8_t _state = 0;
};

template<typename T>
class duplex : public readable<T>, public writeable<T> {
};

} // namespace stream
} // namespace node

#endif // nodecc_stream_h
