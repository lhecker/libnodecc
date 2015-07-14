#ifndef nodecc_buffer_buffer_ref_list_h
#define nodecc_buffer_buffer_ref_list_h


namespace node {

class buffer;


class buffer_ref_list {
public:
	constexpr buffer_ref_list() noexcept : _list(nullptr), _size(0), _capacity(1) {}

	buffer_ref_list(const node::buffer bufs[], size_t bufcnt) noexcept;

	~buffer_ref_list() noexcept;

	void push_front(const buffer& ref) noexcept;
	void emplace_front(buffer&& ref) noexcept;
	void resize(size_t size) noexcept;

	void clear() noexcept;

	void swap(buffer_ref_list& other) noexcept;

private:
	void _push_front(const buffer& ref) noexcept;

	void* _list;
	size_t _size;
	size_t _capacity;
};

} // namespace node

#endif // nodecc_buffer_buffer_ref_list_h
