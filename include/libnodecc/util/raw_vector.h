#ifndef nodecc_util_raw_vector_h
#define nodecc_util_raw_vector_h

#include <atomic>
#include <memory>


namespace node {
namespace util {

template<typename T>
class raw_vector {
public:
	typedef T              value_type;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T*             iterator;
	typedef const T*       const_iterator;

	constexpr raw_vector(T* beg, std::size_t length) : _beg(beg), _end(beg + length) {}

	inline iterator get() noexcept {
		return this->_beg;
	}

	inline iterator begin() noexcept {
		return this->_beg;
	}

	inline const_iterator begin() const noexcept {
		return this->_beg;
	}

	inline const_iterator cbegin() const noexcept {
		return this->_beg;
	}

	inline iterator end() noexcept {
		return this->_end;
	}

	inline const_iterator end() const noexcept {
		return this->_end;
	}

	inline const_iterator cend() const noexcept {
		return this->_end;
	}

private:
	iterator _beg;
	iterator _end;
};

template<typename T, class Deleter = std::default_delete<T>>
class unique_raw_vector : public raw_vector<T> {
public:
	constexpr unique_raw_vector(T* beg, std::size_t length) : raw_vector<T>(beg, length) {}
	unique_raw_vector(T* beg, std::size_t length, const Deleter& d) noexcept : raw_vector<T>(beg, length), _deleter(d) {}
	unique_raw_vector(T* beg, std::size_t length, Deleter&& d) noexcept : raw_vector<T>(beg, length), _deleter(std::move(d)) {}

	~unique_raw_vector() {
		this->_deleter(this->get());
	}

	unique_raw_vector(const unique_raw_vector&) = delete;
	unique_raw_vector(unique_raw_vector&&) = default;

private:
	Deleter _deleter;
};

template<typename T>
class shared_raw_vector : public raw_vector<T> {
public:
	shared_raw_vector(T* beg, std::size_t length) : raw_vector<T>(beg, length) {
		this->_use_count = new std::atomic<std::size_t>(1);
	}

	shared_raw_vector(const shared_raw_vector& other) : raw_vector<T>(other), _use_count(other._use_count) {
		if (this->_use_count) {
			this->_use_count.fetch_add(1, std::memory_order_relaxed);
		}
	}

	shared_raw_vector(shared_raw_vector&& other) : raw_vector<T>(other), _use_count(other._use_count) {
		other._use_count = nullptr;
	}

	~shared_raw_vector() {
		if (this->_use_count && this->_use_count->fetch_sub(1, std::memory_order_release) == 1) {
			std::atomic_thread_fence(std::memory_order_acquire);

			delete this->get();
		}
	}

private:
	std::atomic<std::size_t>* _use_count;
};

} // namespace util
} // namespace node

#endif // nodecc_util_raw_vector_h
