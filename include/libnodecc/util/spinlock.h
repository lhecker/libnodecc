#ifndef nodecc_util_spinlock_h
#define nodecc_util_spinlock_h

#include <atomic>


namespace util {

class spinlock {
public:
	spinlock() : _flag(ATOMIC_FLAG_INIT) {}

	spinlock(const spinlock&) = delete;
	spinlock& operator=(const spinlock&) = delete;

	void lock() {
		while (this->_flag.test_and_set(std::memory_order_acquire));
	}

	bool try_lock() {
		this->lock();
		return true;
	}

	void unlock() {
		this->_flag.clear(std::memory_order_release);
	}

private:
	std::atomic_flag _flag;
};

} // namespace util

#endif // nodecc_util_spinlock_h
