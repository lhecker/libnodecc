#ifndef nodecc_buffer_buffer_h
#define nodecc_buffer_buffer_h

#include "../util/function_traits.h"
#include "buffer_view.h"
#include "literal_string.h"

#include <atomic>


namespace node {

enum class buffer_flags {
	weak = 0,
	copy = 1,
};

class buffer_ref_list;
class mutable_buffer;


/**
 * A immutable buffer, with optional reference counting.
 *
 * Beware that passing a buffer from one thread to another
 * is inherently NOT thread safe and never will be.
 * Please use some other synchronization mechanism, like node::channel.
 */
class buffer : public buffer_view {
	friend class buffer_ref_list;
	friend class mutable_buffer;

public:
	/**
	 * Creates an empty buffer.
	 */
	constexpr buffer() : buffer_view(), _p(nullptr) {}

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer(const buffer& other) noexcept;

	/**
	 * Takes over another buffer.
	 */
	buffer(buffer&& other) noexcept;

	/**
	 * Creates a buffer referring the specified memory area.
	 *
	 * @param data  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param flags node::weak or node::copy
	 */
	explicit buffer(const void* data, std::size_t size, buffer_flags flags = buffer_flags::copy) noexcept;
	explicit buffer(const buffer_view& other, buffer_flags flags = buffer_flags::copy) noexcept : buffer(other.data<void>(), other.size(), flags) {};


	/**
	 * Constructs a buffer from a literal string view.
	 *
	 * This works as a shortcut to the
	 *   buffer(const buffer_view&, buffer_flags)
	 * constructor and allows a buffer being constructed implicitely from
	 * a literal string, since those have a static storage duration.
	 */
	constexpr buffer(const literal_string& other) noexcept : buffer_view(other.data(), other.size()), _p(nullptr) {}

	/**
	 * Takes over another mutable_buffer.
	 */
	buffer(mutable_buffer&& other) noexcept;

	/**
	 * Creates a buffer with the specified size.
	 *
	 * @param size The size of the buffer in bytes.
	 */
	explicit buffer(std::size_t size);

	/**
	 * Creates a buffer referring the specified memory area.
	 *
	 * @param data The base address of the memory area.
	 * @param size The size of the memory area.
	 * @param d    An optional custom deleter callback. The default simply calls free().
	 */
	template<typename T, typename D>
	explicit buffer(const T* data, std::size_t size, D d) noexcept {
		control_base* p = new(std::nothrow) control<T, D>(data, std::forward<D>(d));

		if (p) {
			this->_data = const_cast<void*>(data);
			this->_size = size;
			this->_p = p;
		} else {
			this->_data = nullptr;
			this->_size = 0;
			this->_p = nullptr;
		}
	}


	buffer& operator=(const buffer_view& other);

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer& operator=(const buffer& other);

	/**
	 * Takes over another buffer.
	 */
	buffer& operator=(buffer&& other);


	~buffer();


	inline bool is_strong() const noexcept {
		return this->_p != nullptr;
	}

	inline bool is_weak() const noexcept {
		return this->_p == nullptr;
	}

	std::size_t use_count() const noexcept;

	/**
	 * Swaps the references of this buffer with the other one.
	 */
	void swap(buffer& other) noexcept;

	/**
	 * Swaps the references of this buffer with a mutable_buffer.
	 */
	void swap(mutable_buffer& other) noexcept;

	/**
	 * Releases the buffer and resets it's data and size to zero.
	 */
	void reset();

	/**
	 * Releases the buffer and allocates a new one with the specified size.
	 */
	void reset(size_t size);

	void reset(const buffer_view& other, buffer_flags flags = buffer_flags::copy);

	inline void reset(const void* data, std::size_t size, buffer_flags flags = buffer_flags::copy) {
		this->reset(node::buffer_view(data, size), flags);
	}

	/**
	 * Releases the current buffer and starts managing the given memory area.
	 *
	 * @param data  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param d     An optional custom deleter callback. The default simply calls free().
	 */
	template<typename T, typename D>
	void reset(const T* data, std::size_t size, D d) {
		this->_release();

		control_base* p = new(std::nothrow) control<T, D>(data, std::forward<D>(d));

		if (p) {
			this->_data = const_cast<void*>(data);
			this->_size = size;
			this->_p = p;
		}
	}

	/**
	 * Returns a copy of the buffer, while optionally resizing it.
	 *
	 * @param size If zero (the default), the new size will be equal to the old one.
	 */
	buffer copy(std::size_t size = 0) const;

	/**
	 * Returns a new buffer, pointing to a section of this one.
	 *
	 * The referenced section will start at "start" and end just BEFORE "end".
	 * Or to express this in math: The Interval is [start, end).
	 *
	 * REMEMBER: If the old buffer held a strong reference
	 * to it's backing memory the new one will too.
	 * => The backing memory of the old buffer will NOT be freed!
	 *
	 * Negative indexes (start/end) are counted beginning from the end of the buffer.
	 *
	 * @param start The new buffer is offset by the index start.
	 * @param end   The new buffer is cropped to the index end.
	 */
	buffer slice(std::size_t beg = 0, std::size_t end = npos) const noexcept;

protected:
	class control_base {
	public:
		explicit control_base(const void* base) : base(base), use_count(1) {}
		virtual ~control_base() = default;

		virtual void free() = 0;

		void retain() noexcept;
		void release();

		const void* base;
		std::atomic<unsigned int> use_count;
	};

	template<typename T, typename D>
	class control : public control_base {
	public:
		explicit control(const T* base, D&& d) noexcept : control_base(base), _deleter(std::forward<D>(d)) {}

		void free() override {
			/*
			 * If you get compiler errors in the next 2 lines below, please remember:
			 *   - Your deleter must accept a single parameter. Not more, not less.
			 *   - This parameter must be one to which a void pointer can be cast.
			 */
			this->_deleter(static_cast<T*>(const_cast<void*>(this->base)));
		}

	private:
		D _deleter;
	};

	class default_control : public control_base {
	public:
		using control_base::control_base;

		void free() override {}
	};


	/**
	 * Creates a copy of this buffer in target, while optionally resizing it.
	 */
	void _copy(buffer& target, std::size_t size = 0) const;

	void _reset_unsafe(std::size_t size);
	void _reset_zero() noexcept;

	/**
	 * Retains this buffer, incrementing it's reference count by one,
	 * using std::memory_order_relaxed.
	 */
	void _retain() noexcept;

	/**
	 * Releases this buffer, decrementing it's reference count by one,
	 * using std::memory_order_release and a std::memory_order_acquire fence.
	 */
	void _release();

	control_base* _p;
};

} // namespace node

#endif // nodecc_buffer_buffer_h
