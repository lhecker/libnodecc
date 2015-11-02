#ifndef nodecc_stream_h
#define nodecc_stream_h

#include <array>

#include "callback.h"
#include "events.h"


namespace node {
namespace stream {
namespace detail {

class readable_base {
public:
	static const node::events::type<void()> end_event;

	void resume() {
		if (!this->_is_consuming) {
			this->_resume();
			this->_is_consuming = true;
		}
	}

	void pause() {
		if (this->_is_consuming) {
			this->_pause();
			this->_is_consuming = false;
		}
	}

protected:
	inline bool is_consuming() const noexcept {
		return this->_is_consuming;
	}

	inline bool has_ended() const noexcept {
		return this->_has_ended;
	}

	inline void _reset() {
		this->_is_consuming = false;
		this->_has_ended = false;
	}

	virtual void _resume() = 0;
	virtual void _pause() = 0;

	bool _is_consuming = false;
	bool _has_ended = false;
};


template<typename ChunkT>
class readable : public readable_base {
public:
	static const node::events::type<void(const ChunkT& chunk)> data_event;
};

template<typename ChunkT>
const node::events::type<void(const ChunkT& chunk)> readable<ChunkT>::data_event;

} // namespace detail


template<typename BaseT, typename ChunkT>
class readable : public detail::readable<ChunkT> {
public:
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

protected:
	void _set_reading_ended() {
		if (!this->_has_ended) {
			this->_has_ended = true;
			static_cast<BaseT*>(this)->emit(detail::readable_base::end_event);
		}
	}
};


namespace detail {

class writable_base {
public:
	static const node::events::type<void()> drain_event;

	explicit writable_base(size_t hwm = 16 * 1024, size_t lwm = 4 * 1024) : _hwm(hwm), _lwm(lwm), _wm(0) {}

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

protected:
	inline bool is_writable() const noexcept {
		return this->_is_writable;
	}

	inline bool has_ended() const noexcept {
		return this->_has_ended;
	}

	/*
	 * API for readable implementors below
	 */
	inline void _increase_watermark(size_t n) {
		this->_wm += n;
	}

	inline void _set_writing_ended() {
		this->_is_writable = false;
		this->_has_ended = true;
	}

	inline void _reset() {
		this->_is_regular_level = true;
		this->_is_writable = true;
		this->_has_ended = false;
	}

	size_t _hwm;
	size_t _lwm;
	size_t _wm;
	bool _is_regular_level = true;
	bool _is_writable = true;
	bool _has_ended = false;
};


template<typename ChunkT>
class writable : public writable_base {
public:
	using writable_base::writable_base;

	inline bool write(const ChunkT& chunk) {
		return this->write(&chunk, 1);
	}

	bool write(const ChunkT chunks[], size_t chunkcnt) {
		if (!this->_is_writable) {
			throw std::logic_error("write after end");
		}

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
		if (this->_is_writable) {
			this->_end(chunks, chunkcnt);
			this->_is_regular_level = this->_wm < this->_hwm;
			this->_is_writable = false;
		}

		return this->_is_regular_level;
	}

protected:
	virtual void _write(const ChunkT chunks[], size_t chunkcnt) = 0;
	virtual void _end(const ChunkT chunks[], size_t chunkcnt) = 0;
};

} // namespace detail


template<typename BaseT, typename ChunkT>
class writable : public detail::writable<ChunkT> {
public:
	using detail::writable<ChunkT>::writable;

protected:
	void _decrease_watermark(size_t n) {
		this->_wm = this->_wm > n ? this->_wm - n : 0;

		if (!this->_is_regular_level && this->_wm <= this->_lwm) {
			this->_is_regular_level = true;
			static_cast<BaseT*>(this)->emit(detail::writable_base::drain_event);
		}
	}
};


template<typename BaseT, typename ChunkT>
class duplex : public readable<BaseT, ChunkT>, public writable<BaseT, ChunkT> {
public:
	using writable<BaseT, ChunkT>::writable;

protected:
	inline bool readable_has_ended() const noexcept {
		return readable<BaseT, ChunkT>::has_ended();
	}

	inline bool writable_has_ended() const noexcept {
		return writable<BaseT, ChunkT>::has_ended();
	}
};

} // namespace stream
} // namespace node

#endif // nodecc_stream_h
