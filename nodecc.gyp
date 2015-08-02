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
				'include/libnodecc/buffer/buffer.h',
				'include/libnodecc/buffer/buffer_ref_list.h',
				'include/libnodecc/buffer/buffer_view.h',
				'include/libnodecc/buffer/hashed_buffer.h',
				'include/libnodecc/buffer/hashed_buffer_view.h',
				'include/libnodecc/buffer/hashed_string.h',
				'include/libnodecc/buffer/literal_string.h',
				'include/libnodecc/buffer/mutable_buffer.h',
				'include/libnodecc/buffer/string.h',
				'include/libnodecc/channel.h',
				'include/libnodecc/dns/lookup.h',
				'include/libnodecc/event.h',
				'include/libnodecc/fs/event.h',
				'include/libnodecc/fs/read_stream.h',
				'include/libnodecc/http/_http_date_buffer.h',
				'include/libnodecc/http/client_request.h',
				'include/libnodecc/http/incoming_message.h',
				'include/libnodecc/http/outgoing_message.h',
				'include/libnodecc/http/server.h',
				'include/libnodecc/http/server_response.h',
				'include/libnodecc/loop.h',
				'include/libnodecc/net/server.h',
				'include/libnodecc/net/socket.h',
				'include/libnodecc/stream.h',
				'include/libnodecc/util/_boost_endian.h',
				'include/libnodecc/util/base64.h',
				'include/libnodecc/util/crc32c.h',
				'include/libnodecc/util/endian.h',
				'include/libnodecc/util/fnv.h',
				'include/libnodecc/util/function_traits.h',
				'include/libnodecc/util/math.h',
				'include/libnodecc/util/sha1.h',
				'include/libnodecc/util/timer.h',
				'include/libnodecc/util/uri.h',
				'include/libnodecc/uv/async.h',
				'include/libnodecc/uv/handle.h',
				'include/libnodecc/uv/queue_work.h',
				'include/libnodecc/uv/stream.h',

				'src/buffer/buffer.cc',
				'src/buffer/buffer_ref_list.cc',
				'src/buffer/buffer_view.cc',
				'src/buffer/hashed_buffer.cc',
				'src/buffer/hashed_buffer_view.cc',
				'src/buffer/hashed_string.cc',
				'src/buffer/mutable_buffer.cc',
				'src/buffer/string.cc',
				'src/dns/lookup.cc',
				'src/fs/event.cc',
				'src/http/_http_date_buffer.cc',
				'src/http/_status_codes.h',
				'src/http/client_request.cc',
				'src/http/http_server.cc',
				'src/http/incoming_message.cc',
				'src/http/outgoing_message.cc',
				'src/http/server_response.cc',
				'src/loop.cc',
				'src/net/server.cc',
				'src/net/socket.cc',
				'src/util/base64.cc',
				'src/util/crc32c.cc',
				'src/util/math.cc',
				'src/util/sha1.cc',
				'src/util/timer.cc',
				'src/util/uri.cc',
				'src/uv/async.cc',
				'src/uv/queue_work.cc',
			],
			'dependencies': [
				'deps/json11.gyp:json11',
				'deps/libuv/uv.gyp:libuv',
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
			],
			'xcode_settings': {
				'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
				'CLANG_CXX_LIBRARY': 'libc++',
			},
			'defines': [
				'NOMINMAX',
				'STRICT',
				'UNICODE',
			],
		},
	]
}
