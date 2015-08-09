#ifndef nodecc_buffer_mutable_buffer_h
#define nodecc_buffer_mutable_buffer_h

#include "buffer.h"


/*
 * TODO: Add tests to ensure that _hash is set to zero if the buffer is mutated.
 */
namespace node {

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
	mutable_buffer& append(const node::buffer_view& buf, std::size_t pos = 0, std::size_t count = npos) noexcept;
	mutable_buffer& append(const node::buffer& buf, std::size_t pos = 0, std::size_t count = npos) noexcept;

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

	mutable_buffer slice(std::size_t start = 0, std::size_t end = PTRDIFF_MAX) const noexcept;

	template<typename... Args>
	void reset(Args&&... args) noexcept {
		buffer::reset(std::forward<Args>(args)...);
		this->_capacity = 0;
	}

private:
	/*
	 * Similiar to set_size() but it will never reduce the capacity.
	 * It returns a pointer to the location right after the previous end of the buffer.
	 */
	void* _expand_size(std::size_t size);

	std::size_t _capacity;
};

} // namespace node

#endif // nodecc_buffer_mutable_buffer_h
