#ifndef nodecc_util_buffer_h
#define nodecc_util_buffer_h

#include <string>
#include <vector>
#include <cmath>


namespace util {

enum flags : uint8_t {
	weak   = 0x00,
	strong = 0x01,
	copy   = 0x02,
};

/**
 * A immutable buffer, with optional reference counting.
 *
 * It can either strongly manage a C buffer
 * (one which must be returned with the free() function)
 * using reference counting similiar to std::shared_ptr,
 * or weakly reference some buffer.
 */
class buffer {
public:
	/**
	 * Creates an empty buffer.
	 */
	constexpr buffer() : _p(nullptr), _data(nullptr), _size(0) {};

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer(const buffer &other) noexcept;

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer& operator=(const buffer &other) noexcept;

	/**
	 * Creates a buffer with the specified size.
	 *
	 * @param size The size of the buffer in bytes.
	 */
	explicit buffer(size_t size) noexcept;

	/**
	 * Creates a buffer referring the specified memory area.
	 *
	 * @param base  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	explicit buffer(const void *base, size_t size, util::flags flags) noexcept;


	/**
	 * Creates a buffer referring the specified std::vector.
	 *
	 * @param  vec   The std::vector<T> which should be referred to.
	 * @param  flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	template<typename T>
	explicit buffer(const std::vector<T> &vec, util::flags flags) noexcept : buffer((void*)vec.data(), vec.size(), flags) {}

	/**
	 * Creates a buffer referring the specified std::basic_string.
	 *
	 * @param  str   The std::basic_string<charT, traits, Allocator> which should be referred to.
	 * @param  flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	template<typename charT, typename traits, typename Allocator>
	explicit buffer(const std::basic_string<charT, traits, Allocator> &str, util::flags flags) noexcept : buffer((void*)str.data(), str.size() * sizeof(charT), flags) {}

	/**
	 * Creates a buffer referring the specified vector.
	 *
	 * @param  vec   The Null-terminated byte string which should be referred to.
	 * @param  flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	explicit buffer(const char *str, util::flags flags) noexcept : buffer((void*)str, strlen(str), flags) {}


	~buffer() noexcept;


	/**
	 * Swaps the references of this buffer with the other one.
	 */
	void swap(util::buffer &other) noexcept;

	/**
	 * Releases the buffer and resets it's data and size to zero.
	 */
	void reset() noexcept;

	/**
	 * Releases the current buffer and sets it's new memory it should refer to.
	 *
	 * @param base  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	void reset(const void *base, size_t size, util::flags flags) noexcept;

	/**
	 * Returns a copy of the buffer, while optionally resizing it.
	 *
	 * @param size If zero (the default), the new size will be equal to the old one.
	 */
	util::buffer copy(size_t size = 0) const noexcept;

	/**
	 * Returns a buffer, referencing this buffer, but offset and cropped.
	 *
	 * Negative indexes (start/end) start at the end of the buffer.
	 *
	 * @param start The new buffer is offset by the index start.
	 * @param start The new buffer is cropped to the index end.
	 */
	util::buffer slice(ssize_t start = 0, ssize_t end = SIZE_T_MAX) const noexcept;


	bool is_strong() const noexcept;
	bool is_weak() const noexcept;

	operator void*() const noexcept;
	operator char*() const noexcept;
	operator unsigned char*() const noexcept;

	char& operator[](size_t pos) const noexcept {
		return *(this->data<char>() + pos);
	}

	explicit operator bool() const noexcept;
	size_t use_count() const noexcept;

	uint8_t* get() const noexcept;

	template<typename T = void>
	T* data() const noexcept {
		return reinterpret_cast<T*>(this->_data);
	}

	size_t size() const noexcept;

protected:
	struct control;

	void retain() noexcept;
	void release() noexcept;

	control *_p;
	void *_data;
	size_t _size;
};


template<typename T>
class buffer_allocator {
public:
	typedef T              value_type;
	typedef T*             pointer;
	typedef const T*       const_pointer;
	typedef T&             reference;
	typedef const T&       const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;


	template<typename U>
	struct rebind {
		typedef buffer_allocator<U> other;
	};


	constexpr buffer_allocator() noexcept : _buffer() {}

	buffer_allocator(const buffer_allocator& other) {
		this->_buffer = other._buffer;
	}

	template<typename U>
	buffer_allocator(const buffer_allocator<U>& other) {
		this->_buffer = other._buffer;
	}

	pointer allocate(size_type n, const_pointer hint = 0) {
		this->_buffer = util::buffer(n * sizeof(T));
		return this->_buffer.template data<T>();
	}

	void deallocate(pointer p, size_type n) {
	}

	util::buffer to_buffer() {
		util::buffer buf;
		buf.swap(this->_buffer);
		return buf;
	}

private:
	util::buffer _buffer;
};

} // namespace util

#endif // nodecc_util_buffer_h
