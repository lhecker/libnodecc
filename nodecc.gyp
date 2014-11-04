{
	'targets': [
		{
			'target_name': 'libnodecc',
			'type': '<(library)',
			'include_dirs': [
				'include',
				'deps',
			],
			'sources': [
				'deps/http-parser/http_parser.c',
				'include/libnodecc/buffer.h',
				'include/libnodecc/common.h',
				'include/libnodecc/dns/lookup.h',
				'include/libnodecc/fs/event.h',
				'include/libnodecc/http/client_request.h',
				'include/libnodecc/http/incoming_message.h',
				'include/libnodecc/http/request_response_proto.h',
				'include/libnodecc/http/server.h',
				'include/libnodecc/http/server_response.h',
				'include/libnodecc/loop.h',
				'include/libnodecc/net/server.h',
				'include/libnodecc/net/socket.h',
				'include/libnodecc/util/base64.h',
				'include/libnodecc/util/crc32c.h',
				'include/libnodecc/util/endian.h',
				'include/libnodecc/util/math.h',
				'include/libnodecc/util/notification_queue.h',
				'include/libnodecc/util/timer.h',
				'include/libnodecc/uv/async.h',
				'include/libnodecc/uv/handle.h',
				'include/libnodecc/uv/queue_work.h',
				'include/libnodecc/uv/stream.h',
				'src/buffer.cc',
				'src/dns/lookup.cc',
				'src/fs/event.cc',
				'src/http/client_request.cc',
				'src/http/http_server.cc',
				'src/http/incoming_message.cc',
				'src/http/request_response_proto.cc',
				'src/http/server_response.cc',
				'src/loop.cc',
				'src/net/server.cc',
				'src/net/socket.cc',
				'src/util/base64.cc',
				'src/util/crc32c.cc',
				'src/util/math.cc',
				'src/util/timer.cc',
				'src/uv/async.cc',
				'src/uv/queue_work.cc',
			],
			'dependencies': [
				'deps/json11.gyp:json11',
				'deps/libuv/uv.gyp:libuv',
				'deps/wslay.gyp:wslay',
			],
			'direct_dependent_settings': {
				'include_dirs': [
					'include',
					'deps',
				],
				'xcode_settings': {
					'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
					'CLANG_CXX_LIBRARY': 'libc++',
				},
			},
			'export_dependent_settings': [
				'deps/json11.gyp:json11',
				'deps/libuv/uv.gyp:libuv',
				'deps/wslay.gyp:wslay',
			],
			'xcode_settings': {
				'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
				'CLANG_CXX_LIBRARY': 'libc++',
			},
		},
	]
}
