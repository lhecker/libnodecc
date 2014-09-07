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
	constexpr buffer() : _p(nullptr), _data(nullptr), _size(0) {};

	buffer(const buffer &other) noexcept;
	buffer& operator=(const buffer &other) noexcept;

	explicit buffer(size_t size) noexcept;
	explicit buffer(const void *base, size_t size, util::flags flags) noexcept;


	template<typename T>
	explicit buffer(const std::vector<T> &vec, util::flags flags) noexcept : buffer((void*)vec.data(), vec.size(), flags) {}

	template<typename charT, typename traits, typename Allocator>
	explicit buffer(const std::basic_string<charT, traits, Allocator> &str, util::flags flags) noexcept : buffer((void*)str.data(), str.size() * sizeof(charT), flags) {}

	explicit buffer(const char *str, util::flags flags) noexcept : buffer((void*)str, strlen(str), flags) {}


	~buffer() noexcept;


	void reset() noexcept;
	void reset(const void *base, size_t size, util::flags flags) noexcept;

	/**
	 * Detaches the buffer from the current one.
	 *
	 * @param copy If true (the default), as much data as possible will be copied over to the new buffer.
	 * @param size If zero (the default), the new size will be equal to the old one.
	 *
	 * @return Returns true, if memory for the new instance could be allocated.
	 */
	bool copy(bool copy = true, size_t size = 0) noexcept;


	util::buffer slice(size_t offset = 0, size_t size = SIZE_T_MAX) const noexcept;

	// implemented as a template, so the other slice() method gets preferred
	template<typename T>
	util::buffer slice(const T *data, size_t size = SIZE_T_MAX) const noexcept {
		return this->slice((uint8_t*)data - this->get(), size);
	}


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

	void swap(util::buffer &other) noexcept;

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
