#ifndef nodecc_fs_h
#define nodecc_fs_h


namespace node {
namespace fs {

class readable_stream : node::uv::handle<uv_fs_t>, node::stream::readable<node::buffer, int> {
public:
	explicit reable_stream(const char* path) {
		uv_fs_open(*this, path, OPEN);
	}

	void resume() override {
		uv_fs_read(*this, this->_buffer.data(), this->_buffer.size(), [](uv_fs_t* req) {
		});
	};
	void pause() override {
	};

private:
	node::buffer _buffer;
};

} // namespace fs
} // namespace node

#endif // nodecc_fs_h
