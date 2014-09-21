#ifndef nodecc_queue_work_h
#define nodecc_queue_work_h

#include <functional>

#include "../loop.h"


namespace node {
namespace uv {

typedef std::function<void()> work_cb_t;

void queue_work(node::loop& loop, const work_cb_t& work_cb, const work_cb_t& after_cb = nullptr);

} // namespace uv
} // namespace node

#endif // nodecc_queue_work_h
