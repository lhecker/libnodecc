#ifndef nodecc_loop_h
#define nodecc_loop_h

#include <uv.h>

namespace uv {

class loop {
public:
	explicit loop() noexcept;

	~loop() noexcept;

	void run();
	bool run_once();
	bool run_nowait();

	bool alive();


	operator uv_loop_t*();
	operator const uv_loop_t*() const;

protected:
	uv_loop_t _loop;
};

} // namespace uv

#endif
