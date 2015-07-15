#ifndef nodecc_buffer_buffer_h
#define nodecc_buffer_buffer_h

#include "../util/function_traits.h"
#include "buffer_view.h"

#include <atomic>


namespace node {

enum buffer_flags {
	weak = 0,
	copy = 1,
};

class mutable_buffer;
class buffer_ref_list;


/**
 * A immutable buffer, with optional reference counting.
 *
 * Beware that passing a buffer from one thread to another
 * is inherently NOT thread safe and never will be.
 * Please use some other synchronization mechanism, like node::channel.
 */
class buffer : public buffer_view {
	friend class node::mutable_buffer;
	friend class node::buffer_ref_list;

public:
	/**
	 * Creates an empty buffer.
	 */
	constexpr buffer() : buffer_view(), _p(nullptr) {}

	constexpr buffer(buffer_view&& other) : buffer_view(other), _p(nullptr) {}
	constexpr buffer(const buffer_view& other) : buffer_view(other), _p(nullptr) {}

	/**
	 * Takes over another buffer.
	 */
	buffer(buffer&& other) noexcept;

	/**
	 * Takes over another mutable_buffer.
	 */
	buffer(mutable_buffer&& other) noexcept;

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer(const buffer& other) noexcept;

	buffer& operator=(buffer_view&& other) noexcept;
	buffer& operator=(const buffer_view& other) noexcept;

	/**
	 * Takes over another buffer.
	 */
	buffer& operator=(buffer&& other) noexcept;

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer& operator=(const buffer& other) noexcept;

	/**
	 * Creates a buffer with the specified size.
	 *
	 * @param size The size of the buffer in bytes.
	 */
	explicit buffer(std::size_t size) noexcept;

	/**
	 * Creates a buffer referring the specified memory area.
	 *
	 * @param data The base address of the memory area.
	 * @param size The size of the memory area.
	 * @param d    An optional custom deleter callback. The default simply calls free().
	 */
	template<typename D>
	explicit buffer(const void* data, std::size_t size, D d) noexcept {
		control_base* p = new(std::nothrow) control<D>(data, std::forward<D>(d));

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

	/**
	 * Creates a buffer referring the specified memory area.
	 *
	 * @param data  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param flags node::weak or node::copy
	 */
	explicit buffer(const void* data, std::size_t size, buffer_flags flags = node::copy) noexcept;

	explicit buffer(const buffer_view other, buffer_flags flags = node::copy) noexcept : buffer(other.data(), other.size(), flags) {};

	template<typename charT>
	explicit buffer(const charT* str, buffer_flags flags = node::copy) noexcept : buffer(const_cast<charT*>(str), std::char_traits<charT>::length(str) * sizeof(charT), flags) {}

	template<typename charT, typename traits, typename Allocator>
	explicit buffer(const std::basic_string<charT, traits, Allocator>& str, buffer_flags flags = node::copy) noexcept : buffer(str.data(), str.size() * sizeof(charT), flags) {}

	~buffer() noexcept;


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
	void reset() noexcept;

	/**
	 * Releases the buffer and allocates a new one with the specified size.
	 */
	void reset(size_t size) noexcept;

	/**
	 * Releases the current buffer and starts managing the given memory area.
	 *
	 * @param data  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param d     An optional custom deleter callback. The default simply calls free().
	 */
	template<typename D>
	void reset(const void* data, std::size_t size, D d) noexcept {
		this->_release();

		control_base* p = new(std::nothrow) control<D>(data, std::forward<D>(d));

		if (p) {
			this->_data = const_cast<void*>(data);
			this->_size = size;
			this->_p = p;
		}
	}

	void reset(const void* data, std::size_t size, buffer_flags flags = node::copy) noexcept;
	void reset(const buffer_view other, buffer_flags flags = node::copy) noexcept;
	void reset(const char str[], buffer_flags flags = node::copy) noexcept;

	/**
	 * Returns a copy of the buffer, while optionally resizing it.
	 *
	 * @param size If zero (the default), the new size will be equal to the old one.
	 */
	buffer copy(std::size_t size = 0) const noexcept;

	/**
	 * Returns a buffer, referencing this buffer, but offset and cropped.
	 *
	 * Negative indexes (start/end) start at the end of the buffer.
	 *
	 * @param start The new buffer is offset by the index start.
	 * @param end   The new buffer is cropped to the index end.
	 */
	buffer slice(std::ptrdiff_t start = 0, std::ptrdiff_t end = PTRDIFF_MAX) const noexcept;


	int compare(std::size_t pos1, std::size_t size1, const void* data2, std::size_t size2) const noexcept;


	inline int compare(std::size_t size1, const void* data2, std::size_t size2) const noexcept {
		return this->compare(0, size1, data2, size2);
	}

	inline int compare(const void* data2, std::size_t size2) const noexcept {
		return this->compare(0, this->size(), data2, size2);
	}

	inline int compare(const buffer& other) const noexcept {
		return this->compare(0, this->size(), other.get(), other.size());
	}

	template<typename charT>
	inline int compare(const charT* str) const noexcept {
		return this->compare(static_cast<const void*>(str), std::char_traits<charT>::length(str));
	}

protected:
	class control_base {
	public:
		explicit control_base(const void* base) : base(base), use_count(1) {}
		virtual ~control_base() = default;

		virtual void free() = 0;

		void retain() noexcept;
		void release() noexcept;

		const void* base;
		std::atomic<uintptr_t> use_count;
	};

	template<typename D>
	class control : public control_base {
	public:
		explicit control(const void* base, D d) noexcept : control_base(base), _deleter(std::move(d)) {}

		void free() override {
			/*
			 * If you get compiler errors in the next 2 lines below, please remember:
			 *   - Your deleter must accept a single parameter. Not more, not less.
			 *   - This parameter must be one to which a void pointer can be cast.
			 */
			this->_deleter(static_cast<typename node::util::function_traits<D>::template arg<0>::type>(const_cast<void*>(this->base)));
		}

	private:
		D _deleter;
	};

	/**
	 * Creates a copy of this buffer in target, while optionally resizing it.
	 */
	void copy(buffer& target, std::size_t size = 0) const noexcept;

private:
	void _reset_unreleased() noexcept;
	void _reset_unsafe(std::size_t size) noexcept;

	/**
	 * Retains this buffer, incrementing it's reference count by one,
	 * using std::memory_order_relaxed.
	 */
	void _retain() noexcept;

	/**
	 * Releases this buffer, decrementing it's reference count by one,
	 * using std::memory_order_release and a std::memory_order_acquire fence.
	 */
	void _release() noexcept;

	control_base* _p;
};

} // namespace node


template<>
struct std::hash<node::buffer> {
	std::size_t operator()(const node::buffer& buf) const {
		std::size_t x = std::size_t(buf.data());
		return x + (x >> 3);
	}
};

#endif // nodecc_buffer_buffer_h
