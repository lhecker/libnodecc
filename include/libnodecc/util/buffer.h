#ifndef nodecc_util_buffer_h
#define nodecc_util_buffer_h

#include <cmath>
#include <functional>
#include <string>
#include <vector>


namespace util {

enum flags : uint8_t {
	weak   = 0x00,
	strong = 0x01,
	copy   = 0x02,
};

class string;

/**
 * A immutable buffer, with optional reference counting.
 *
 * It can either strongly manage a C buffer
 * (that is, one which was allocated with malloc etc.)
 * using reference counting similiar to std::shared_ptr,
 * or weakly reference some buffer.
 */
class buffer {
public:
	/**
	 * Creates an empty buffer.
	 */
	constexpr buffer() : _p(nullptr), _data(nullptr), _size(0) {}

	/**
	 * Takes over another buffer.
	 */
	buffer(util::buffer&& other) noexcept;

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	buffer(const util::buffer& other) noexcept;

	/**
	 * Retains another buffer, while referring to it's data.
	 */
	util::buffer& operator=(const util::buffer& other) noexcept;

	/**
	 * Creates a buffer with the specified size.
	 *
	 * @param size The size of the buffer in bytes.
	 */
	explicit buffer(std::size_t size) noexcept;

	/**
	 * Creates a buffer referring the specified memory area.
	 *
	 * @param base  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	explicit buffer(const void* base, std::size_t size, util::flags flags) noexcept;


	/**
	 * Creates a buffer referring the specified std::vector.
	 *
	 * @param  vec   The std::vector<T> which should be referred to.
	 * @param  flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	template<typename T>
	explicit buffer(const std::vector<T>& vec, util::flags flags) noexcept : util::buffer((void*)vec.data(), vec.size(), flags) {}

	/**
	 * Creates a buffer referring the specified std::basic_string.
	 *
	 * @param  str   The std::basic_string<charT, traits, Allocator> which should be referred to.
	 * @param  flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	template<typename charT, typename traits, typename Allocator>
	explicit buffer(const std::basic_string<charT, traits, Allocator>& str, util::flags flags) noexcept : util::buffer((void*)str.data(), str.size() * sizeof(charT), flags) {}

	/**
	 * Creates a buffer referring the specified vector.
	 *
	 * @param  vec   The Null-terminated byte string which should be referred to.
	 * @param  flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	template<typename charT>
	explicit buffer(const charT* str, util::flags flags) noexcept : util::buffer((void*)str, std::char_traits<charT>::length(str), flags) {}


	~buffer() noexcept;


	/**
	 * Swaps the references of this buffer with the other one.
	 */
	void swap(util::buffer& other) noexcept;

	void assign(const util::buffer& other);
	void assign(util::buffer&& other);

	/**
	 * Releases the buffer and resets it's data and size to zero.
	 */
	void reset() noexcept;

	/**
	 * Releases the buffer and creates a new one with the specified size.
	 */
	void reset(size_t size) noexcept;

	/**
	 * Releases the current buffer and sets it's new memory it should refer to.
	 *
	 * @param base  The base address of the memory area.
	 * @param size  The size of the memory area.
	 * @param flags Specifies how the memory is referred. E.g. weak, strong, or copy.
	 */
	void reset(const void* base, std::size_t size, util::flags flags) noexcept;

	/**
	 * Returns a copy of the buffer, while optionally resizing it.
	 *
	 * @param size If zero (the default), the new size will be equal to the old one.
	 */
	util::buffer copy(std::size_t size = 0) const noexcept;

	/**
	 * Returns a buffer, referencing this buffer, but offset and cropped.
	 *
	 * Negative indexes (start/end) start at the end of the buffer.
	 *
	 * @param start The new buffer is offset by the index start.
	 * @param start The new buffer is cropped to the index end.
	 */
	util::buffer slice(std::ptrdiff_t start = 0, std::ptrdiff_t end = SSIZE_MAX) const noexcept;


	bool is_strong() const noexcept;
	bool is_weak() const noexcept;

	operator void*() const noexcept;
	operator char*() const noexcept;
	operator unsigned char*() const noexcept;
	char& operator[](std::size_t pos) const noexcept;

	explicit operator bool() const noexcept;
	std::size_t use_count() const noexcept;

	uint8_t* get() const noexcept;

	template<typename T = uint8_t>
	T* data() const noexcept { return reinterpret_cast<T*>(this->_data); }

	std::size_t size() const noexcept;


	int compare(std::size_t pos1, std::size_t size1, const void* data2, std::size_t size2) const noexcept;
	int compare(std::size_t size1, const void* data2, std::size_t size2) const noexcept;
	int compare(const void* data2, std::size_t size2) const noexcept;
	int compare(const util::buffer& other) const noexcept;

	template<typename charT>
	int compare(const charT* str) const noexcept { return this->compare(static_cast<const void*>(str), std::char_traits<charT>::length(str)); }


	friend bool operator==(const util::buffer& lhs, const util::buffer& rhs) noexcept;
	friend bool operator!=(const util::buffer& lhs, const util::buffer& rhs) noexcept;

protected:
	struct control;

	/**
	 * Creates a copy of this buffer in target, while optionally resizing it.
	 *
	 * This method DOES NOT release() target!.
	 */
	void copy(util::buffer& target, std::size_t size = 0) const noexcept;

	/**
	 * Retains this buffer, incrementing it's reference count by one,
	 * using std::memory_order_relaxed.
	 */
	void retain() noexcept;

	/**
	 * Releases this buffer, decrementing it's reference count by one,
	 * using std::memory_order_release and a std::memory_order_acquire fence.
	 */
	void release() noexcept;

	control* _p;
	void* _data;
	std::size_t _size;
};

} // namespace util


template<>
struct std::hash<util::buffer> {
	std::size_t operator()(const util::buffer& buf) const {
		std::size_t x = size_t(buf.data());
		return x + (x >> 3);
	}
};

template<>
struct std::equal_to<util::buffer> {
	bool operator()(const util::buffer& lhs, const util::buffer& rhs) const {
		return lhs.data() == rhs.data();
	}
};

#endif // nodecc_util_buffer_h
