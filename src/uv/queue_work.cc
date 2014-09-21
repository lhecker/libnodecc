#include "libnodecc/uv/queue_work.h"


namespace {

struct work_t {
	explicit work_t(const node::uv::work_cb_t& work_cb, const node::uv::work_cb_t& after_cb) : work_cb(work_cb), after_cb(after_cb) {}

	uv_work_t req;
	node::uv::work_cb_t work_cb;
	node::uv::work_cb_t after_cb;
};

}


namespace node {
namespace uv {

void queue_work(node::loop& loop, const work_cb_t& work_cb, const work_cb_t& after_cb) {
	work_t *pack = new work_t(work_cb, after_cb);
	uv_queue_work(loop, &pack->req, [](uv_work_t* req) {
		auto self = reinterpret_cast<work_t*>(req);
		self->work_cb();
	}, [](uv_work_t* req, int status) {
		auto self = reinterpret_cast<work_t*>(req);

		if (self->after_cb) {
			self->after_cb();
		}

		delete self;
	});
}

} // namespace node
} // namespace uv
