#ifndef nodecc_util_buffer_h
#define nodecc_util_buffer_h

#include <string>
#include <vector>


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

	explicit buffer(const std::string &str, util::flags flags) noexcept : buffer((void*)str.data(), str.size(), flags) {}

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

	util::buffer slice(size_t offset = 0, size_t size = 0) const noexcept;
	util::buffer slice(const void *data, size_t size = 0) const noexcept;


	bool is_strong() const noexcept;
	bool is_weak() const noexcept;

	operator void*() const noexcept;
	operator char*() const noexcept;
	operator unsigned char*() const noexcept;

	explicit operator bool() const noexcept;
	size_t use_count() const noexcept;

	uint8_t *get() const noexcept;

	template<typename T = void>
	T *data() const noexcept {
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

} // namespace util

#endif // nodecc_util_buffer_h
