#ifndef nodecc_stream_h
#define nodecc_stream_h

#include "callback.h"
#include "signal.h"


namespace node {
namespace stream {

template <typename E, typename T>
class writable;

template <typename E, typename T>
class readable;

} // namespace stream


template<typename E, typename T>
static void pipe(stream::readable<E, T>& from, stream::writable<E, T>& to, bool end = true) {
	from.data_callback.connect([&from, &to](const T chunks[], size_t chunkcnt) {
		printf("data_callback\n");

		if (to.write(chunks, chunkcnt)) {
			from.pause();
		}
	});

	from.error_callback.connect([&to](E err) {
		to.error_callback.emit(err);
	});

	to.drain_callback.connect([&from]() {
		printf("drain_callback\n");

		from.resume();
	});

	if (end) {
		from.end_callback.connect([&to]() {
			printf("end_callback\n");

			to.end();
		});
	}

	if (!to.is_flooded()) {
		from.resume();
	}
}

template<typename X, typename Y, typename = typename std::enable_if<std::is_same<typename X::exception_type, typename Y::exception_type>::value && std::is_same<typename X::chunk_type, typename Y::chunk_type>::value>::type>
static void pipe(const std::shared_ptr<X>& from, const std::shared_ptr<Y>& to, bool end = true) {
	from->data_callback.connect([from, to](const typename X::chunk_type chunks[], size_t chunkcnt) {
		printf("data_callback\n");

		if (to->write(chunks, chunkcnt)) {
			from->pause();
		}
	});

	from->error_callback.connect([to](typename X::exception_type err) {
		to->error_callback.emit(err);
	});

	to->drain_callback.connect([from]() {
		printf("drain_callback\n");

		from->resume();
	});

	if (end) {
		from->end_callback.connect([to]() {
			printf("end_callback\n");

			to->end();
		});
	}

	if (!to->is_flooded()) {
		from->resume();
	}
}


namespace stream {
namespace detail {

template<typename E, typename T>
class base {
public:
	typedef E exception_type;
	typedef T chunk_type;

	node::callback<void(E err)> error_callback;
};

} // namespace detail


template<typename E, typename T>
class readable : public virtual detail::base<E, T> {
public:
	friend class writable<E, T>;

	virtual void resume() = 0;
	virtual void pause() = 0;

	template<typename To>
	void pipe(const writable<E, T>& to, bool end = true) {
		node::pipe(this, to, end);
	}

	void _destroy() {
		printf("_destroy readable\n");

		this->error_callback.clear();

		this->data_callback.clear();
		this->end_callback.clear();
	}

	node::callback<void(const T chunks[], size_t chunkcnt)> data_callback;
	node::callback<void()> end_callback;
};

template<typename E, typename T>
class writable : public virtual detail::base<E, T> {
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

	inline bool is_flooded() const {
		return this->_was_flooded;
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
		return this->end(nullptr, 0);
	}

	inline bool end(const T& chunk) {
		return this->end(&chunk, 1);
	}

	bool end(const T chunks[], size_t chunkcnt) {
		this->_end(chunks, chunkcnt);
		this->_was_flooded = this->_wm >= this->_hwm;
		return this->_was_flooded;
	}

	node::callback<void()> drain_callback;

protected:
	inline void increase_watermark(size_t n) {
		this->_hwm += n;
	}

	void decrease_watermark(size_t n) {
		this->_wm = this->_wm > n ? this->_wm - n : 0;

		if (this->_was_flooded && this->_wm <= this->_lwm) {
			this->drain_callback.emit();
			this->_was_flooded = false;
		}
	}

	void _destroy() {
		printf("_destroy writable\n");

		this->error_callback.clear();

		this->drain_callback.clear();
	}

	virtual void _write(const T chunks[], size_t chunkcnt) = 0;
	virtual void _end(const T chunks[], size_t chunkcnt) = 0;

private:
	size_t _hwm;
	size_t _lwm;
	size_t _wm;
	bool _was_flooded;
};

template<typename E, typename T>
class duplex : public readable<E, T>, public writable<E, T> {
protected:
	void _destroy() {
		printf("_destroy duplex\n");

		this->error_callback.clear();

		this->data_callback.clear();
		this->end_callback.clear();

		this->drain_callback.clear();
	}
};

} // namespace stream
} // namespace node

#endif // nodecc_stream_h
