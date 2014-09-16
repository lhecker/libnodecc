#ifndef nodecc_queue_work_h
#define nodecc_queue_work_h

#include <functional>
#include "loop.h"


namespace uv {

typedef std::function<void()> work_cb_t;

void queue_work(uv::loop& loop, const work_cb_t& work_cb, const work_cb_t& after_cb = nullptr);

} // namespace uv

#endif // nodecc_queue_work_h
