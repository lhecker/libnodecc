#ifndef nodecc_buffer_h
#define nodecc_buffer_h

#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "util/function_traits.h"


namespace node {

enum buffer_flags {
	weak = 0,
	copy = 1,
};

class buffer;
class mutable_buffer;


/**
 * A immutable view on a memory area.
 *
 * This class can be used as a parameter for any function that deals with
 * memory areas, but only deal with them as long as the function call lasts.
 * This leads to some optimizations: This class can thus be initialized with
 * other buffers, strings and pointers, without worrying about whether
 */
class buffer_view {
	friend class node::buffer;
	friend class node::mutable_buffer;

public:
	constexpr buffer_view() : _data(nullptr), _size(0) {}
	constexpr buffer_view(const buffer_view& other) : _data(other._data), _size(other._size) {}

	buffer_view& operator=(const buffer_view& other);

	explicit buffer_view(const void* data, std::size_t size) noexcept;

	template<typename T>
	explicit buffer_view(const std::vector<T>& vec) noexcept : buffer_view(vec.data(), vec.size()) {}

	template<typename charT>
	explicit buffer_view(const charT* str) noexcept : buffer_view(const_cast<charT*>(str), std::char_traits<charT>::length(str) * sizeof(charT)) {}

	template<typename charT, typename traits, typename Allocator>
	explicit buffer_view(const std::basic_string<charT, traits, Allocator>& str) noexcept : buffer_view(str.data(), str.size() * sizeof(charT)) {}


	template<typename T = uint8_t>
	inline T* data() const noexcept {
		return static_cast<T*>(this->_data);
	}

	inline operator void*() const noexcept {
		return this->data<void>();
	}

	inline operator char*() const noexcept {
		return this->data<char>();
	}

	inline operator unsigned char*() const noexcept {
		return this->data<unsigned char>();
	}

	inline uint8_t& operator[](std::size_t pos) noexcept {
		return this->data<uint8_t>()[pos];
	}

	inline const uint8_t& operator[](std::size_t pos) const noexcept {
		return this->data<uint8_t>()[pos];
	}

	inline operator bool() const noexcept {
		return this->_data;
	}

	inline bool empty() const noexcept {
		return !this->_data;
	}

	inline uint8_t* get() const noexcept {
		return this->data<uint8_t>();
	}

	inline std::size_t size() const noexcept {
		return this->_size;
	}

	template<typename CharT = char, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
	inline std::basic_string<CharT, Traits, Allocator> to_string() const {
		return std::basic_string<CharT, Traits, Allocator>(reinterpret_cast<CharT*>(this->_data), this->_size);
	}

	friend bool operator==(const buffer_view& lhs, const buffer_view& rhs) noexcept;
	friend bool operator!=(const buffer_view& lhs, const buffer_view& rhs) noexcept;

private:
	void* _data;
	std::size_t _size;
};


/**
 * A immutable buffer, with optional reference counting.
 *
 * It can either strongly manage a C buffer
 * (that is, one which was allocated with malloc etc.)
 * using reference counting similiar to std::shared_ptr,
 * or weakly reference some buffer.
 */
class buffer : public buffer_view {
	friend class node::mutable_buffer;

public:
	/**
	 * Creates an empty buffer.
	 */
	constexpr buffer() : buffer_view(), _p(nullptr) {}

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
		return this->_p;
	}

	inline bool is_weak() const noexcept {
		return !this->_p;
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
		constexpr control_base(const void* base) : base(base), use_count(1) {}
		virtual ~control_base() = default;

		virtual void free() = 0;

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



class mutable_buffer : public node::buffer {
	friend class node::buffer;

public:
	explicit mutable_buffer() noexcept;
	explicit mutable_buffer(std::size_t capacity) noexcept;

	mutable_buffer(node::buffer&& other) noexcept;
	mutable_buffer& operator=(node::buffer&& other) noexcept;

	mutable_buffer(const node::buffer& other) noexcept;
	mutable_buffer& operator=(const node::buffer& other) noexcept;

	mutable_buffer(mutable_buffer&& other) noexcept;
	mutable_buffer& operator=(mutable_buffer&& other) noexcept;

	mutable_buffer(const mutable_buffer& other) noexcept;
	mutable_buffer& operator=(const mutable_buffer& other) noexcept;

	mutable_buffer& append(const void* data, std::size_t size) noexcept;
	mutable_buffer& append(const node::buffer& buf, std::size_t pos = 0, std::size_t count = SIZE_T_MAX) noexcept;

	template<typename charT>
	void push_back(charT ch) noexcept {
		charT* p = static_cast<charT*>(this->_expand_size(sizeof(charT)));

		if (p) {
			*p = ch;
		}
	}

	template<typename charT>
	mutable_buffer& append(const charT* data) noexcept {
		this->append(static_cast<const void*>(data), strlen(data));
		return *this;
	}

	template<typename charT>
	mutable_buffer& append(const charT* data, std::size_t count) noexcept {
		this->append(static_cast<const void*>(data), count * sizeof(charT));
		return *this;
	}

	template<typename charT, typename traits, typename Allocator>
	mutable_buffer& append(const std::basic_string<charT, traits, Allocator>& str) noexcept {
		this->append(str.data(), str.size() * sizeof(charT));
		return *this;
	}

	template<typename charT, typename traits, typename Allocator>
	mutable_buffer& append(const std::basic_string<charT, traits, Allocator>& str, std::size_t pos, std::size_t count) noexcept {
		if (pos >= str.size()) {
			throw std::out_of_range("mutable_buffer");
		}

		this->append(str.data() + pos, std::min(count, str.size() - pos) * sizeof(charT));
		return *this;
	}

	mutable_buffer& append_number(std::size_t n, uint8_t base = 10);

	std::size_t capacity() const noexcept;
	void set_capacity(std::size_t capacity) noexcept;
	void set_size(std::size_t size) noexcept;

	void clear() noexcept;
	void reset() noexcept;

private:
	/*
	 * Similiar to set_size() but it will never reduce the capacity.
	 * It returns a pointer to the location right after the previous end of the buffer.
	 */
	void* _expand_size(std::size_t size);

	std::size_t _capacity;
};

} // namespace node


template<>
struct std::hash<node::buffer_view> {
	std::size_t operator()(const node::buffer& buf) const {
		std::size_t x = std::size_t(buf.data());
		return x + (x >> 3);
	}
};

template<>
struct std::equal_to<node::buffer_view> {
	bool operator()(const node::buffer_view& lhs, const node::buffer_view& rhs) const {
		return lhs.data() == rhs.data() && lhs.size() == rhs.size();
	}
};

#endif // nodecc_buffer_h
