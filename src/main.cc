#include <iostream>
#include <cstdint>
#include <cassert>
#include <zlib/zlib.h>


int main(void) {
	uint8_t *in  = (uint8_t *)"abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789";
	uint8_t *ref = (uint8_t *)"abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789";
	size_t avail_in = strlen((const char *)in);
	size_t avail_out = avail_in + 100;

	uint8_t *out = (uint8_t *)calloc(1, avail_in);
	uint8_t *ou2 = (uint8_t *)calloc(1, avail_out);
	
	for (size_t i = 0; i < avail_in; i++) {
		out[i] = in[i] - ref[i];
	}

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm, Z_DEFAULT_COMPRESSION);

	strm.avail_in = (uInt)avail_in;
	strm.next_in = out;

	strm.avail_out = (uInt)avail_out;
	strm.next_out = ou2;

	deflate(&strm, Z_FINISH);
	assert(strm.avail_in == 0);

	deflateEnd(&strm);

	std::cout << "avail_in: " << avail_in << std::endl;
	std::cout << "out:      " << (avail_out - strm.avail_out) << std::endl;

	std::cin.get();
}


/*#include <cassert>
#include <iostream>
#include <libnodecc/http/server.h>
#include <libnodecc/http/client_request.h>


int main(void) {
	uv_loop_t *loop = uv_default_loop();

	uv_signal_t signal;
	uv_signal_init(loop, &signal);
	uv_unref((uv_handle_t *)&signal);
	uv_signal_start(&signal, [](uv_signal_t *handle, int signum) {
		uv_stop(handle->loop);
	}, SIGINT);

	http::server server(loop);
	server.on_request = [](http::incoming_message &req, http::server_response &res) {
		res.setHeader("content-type", "text/plain");
		res.setHeader("connection", "keep-alive");
		res.end();
	};
	server.listen(8080);

	http::client_request client(loop);
	client.on_read = [](ssize_t nread, const uv_buf_t *buf) {
		std::cout.write(buf->base, nread);
	};

	client.hostname = "www.google.de";
	client.method = "GET";
	client.path = "/";
	client.end();


	uv_run(loop, UV_RUN_DEFAULT);

	system("pause");

	return 0;
}*/
