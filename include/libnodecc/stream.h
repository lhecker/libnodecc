#ifndef nodecc_stream_h
#define nodecc_stream_h

#include <array>

#include "callback.h"
#include "signal.h"


namespace node {
namespace stream {
namespace detail {

template<typename ErrorT, typename ChunkT>
class base {
public:
	typedef ErrorT error_type;
	typedef ChunkT chunk_type;

	node::signal<void(ErrorT err)> error_signal;

protected:
	void _destroy() {
		this->error_signal.clear();
	}
};


template<typename BaseT, typename ErrorT, typename ChunkT>
class readable {
public:
	void resume() {
		if (!this->_is_flowing) {
			this->_resume();
			this->_is_flowing = true;
		}
	}

	void pause() {
		if (this->_is_flowing) {
			this->_pause();
			this->_is_flowing = false;
		}
	}

	template<typename To>
	To& pipe(To& to, bool end = true) {
		auto from = static_cast<BaseT*>(this)->template shared_from_this<BaseT>();
		std::array<void*, 3> from_iters;

		from_iters[0] = from->data_signal.connect([from, to](const ChunkT chunks[], size_t chunkcnt) {
			if (!to->write(chunks, chunkcnt)) {
				from->pause();
			}
		});

		from_iters[1] = from->error_signal.connect([to](typename To::error_type err) {
			to->error_signal.emit(err);
		});

		to->drain_signal.connect([from]() {
			from->resume();
		});

		if (to->is_regular_level()) {
			from->resume();
		}

		if (end) {
			from_iters[2] = from->end_signal.connect([to]() {
				to->end();
			});
		} else {
			from_iters[2] = nullptr;
		}

		to->destroy_signal.connect([from, from_iters]() {
			from->data_signal.disconnect(from_iters[0]);
			from->error_signal.disconnect(from_iters[1]);
			from->end_signal.disconnect(from_iters[2]);
		});

		return to;
	}

	node::signal<void(const ChunkT chunks[], size_t chunkcnt)> data_callback;
	node::signal<void()> end_signal;

protected:
	void _destroy() {
		this->data_callback.clear();
		this->end_signal.clear();
	}

	virtual void _resume() = 0;
	virtual void _pause() = 0;

private:
	bool _is_flowing = false;
};

template<typename BaseT, typename ErrorT, typename ChunkT>
class writable {
public:
	explicit writable(size_t hwm = 16 * 1024, size_t lwm = 4 * 1024) : _hwm(hwm), _lwm(lwm), _wm(0), _is_regular_level(true), _is_running(true) {}

	/*
	* TODO: add callbacks to write() and writev()
	* BUT:
	* A developer might write something like this:
	*   void cb() { writer.write(buffer, cb); }
	*   cb();
	* which might lead to infinite recursion, if the write() function calls the cb synchronously.
	* (This is for instance the case for node::stream<ChunkT> which uses the synchronous uv_try_write.)
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

	inline bool is_regular_level() const {
		return this->_is_regular_level;
	}

	inline bool is_flooded() const {
		return !this->_is_regular_level;
	}

	inline bool write(const ChunkT& chunk) {
		return this->write(&chunk, 1);
	}

	bool write(const ChunkT chunks[], size_t chunkcnt) {
		this->_write(chunks, chunkcnt);
		this->_is_regular_level = this->_wm < this->_hwm;
		return this->_is_regular_level;
	}

	inline bool end() {
		return this->end(nullptr, 0);
	}

	inline bool end(const ChunkT& chunk) {
		return this->end(&chunk, 1);
	}

	bool end(const ChunkT chunks[], size_t chunkcnt) {
		if (this->_is_running) {
			this->_end(chunks, chunkcnt);
			this->_is_running = false;
			this->_is_regular_level = this->_wm < this->_hwm;
		}

		return this->_is_regular_level;
	}

	node::signal<void()> drain_signal;

protected:
	inline void increase_watermark(size_t n) {
		this->_hwm += n;
	}

	void decrease_watermark(size_t n) {
		this->_wm = this->_wm > n ? this->_wm - n : 0;

		if (!this->_is_regular_level && this->_wm <= this->_lwm) {
			this->_is_regular_level = true;
			this->drain_signal.emit();
		}
	}

	void _destroy() {
		this->drain_signal.clear();
	}

	virtual void _write(const ChunkT chunks[], size_t chunkcnt) = 0;
	virtual void _end(const ChunkT chunks[], size_t chunkcnt) = 0;

private:
	size_t _hwm;
	size_t _lwm;
	size_t _wm;
	bool _is_regular_level;
	bool _is_running;
};

} // namespace detail


template<typename BaseT, typename ErrorT, typename ChunkT>
class readable : public detail::base<ErrorT, ChunkT>, public detail::readable<BaseT, ErrorT, ChunkT> {
protected:
	void _destroy() {
		detail::base<ErrorT, ChunkT>::_destroy();
		detail::readable<BaseT, ErrorT, ChunkT>::_destroy();
	}
};


template<typename BaseT, typename ErrorT, typename ChunkT>
class writable : public detail::base<ErrorT, ChunkT>, public detail::writable<BaseT, ErrorT, ChunkT> {
protected:
	void _destroy() {
		detail::base<ErrorT, ChunkT>::_destroy();
		detail::writable<BaseT, ErrorT, ChunkT>::_destroy();
	}
};


template<typename BaseT, typename ErrorT, typename ChunkT>
class duplex : public detail::base<ErrorT, ChunkT>, public detail::readable<BaseT, ErrorT, ChunkT>, public detail::writable<BaseT, ErrorT, ChunkT> {
protected:
	void _destroy() {
		detail::base<ErrorT, ChunkT>::_destroy();
		detail::readable<BaseT, ErrorT, ChunkT>::_destroy();
		detail::writable<BaseT, ErrorT, ChunkT>::_destroy();
	}
};

} // namespace stream
} // namespace node

#endif // nodecc_stream_h
