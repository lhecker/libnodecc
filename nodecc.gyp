{
	'target_defaults': {
		'conditions': [
			['OS != "win"', {
				'defines': [
					'_LARGEFILE_SOURCE',
					'_FILE_OFFSET_BITS=64',
				],
			}],
		],
	},

	'targets': [
		{
			'target_name': 'libnodecc',
			'type': '<(uv_library)',
			'include_dirs': [
				'include',
				'src/http/http-parser',
			],
			'sources': [
				'include/libnodecc/common.h',
				'include/libnodecc/dns/dns.h',
				'include/libnodecc/http/client_request.h',
				'include/libnodecc/http/incoming_message.h',
				'include/libnodecc/http/server.h',
				'include/libnodecc/http/server_response.h',
				'include/libnodecc/net/server.h',
				'include/libnodecc/net/socket.h',
				'include/libnodecc/util/spinlock.h',
				'include/libnodecc/uv/handle.h',
				'include/libnodecc/uv/stream.h',
				'src/dns/dns.cc',
				'src/http/client_request.cc',
				'src/http/http-parser/http_parser.c',
				'src/http/http_server.cc',
				'src/http/incoming_message.cc',
				'src/http/server_response.cc',
				'src/net/server.cc',
				'src/net/socket.cc',
				'src/uv/handle.cc',
				'src/uv/stream.cc',
			],
			'dependencies': [
				'deps/libuv/uv.gyp:libuv',
			],
			'conditions': [
				[ 'OS=="win"', {
					'defines': [
						'_WIN32_WINNT=0x0600',
					],
				}, { # Not Windows i.e. POSIX
					'cflags': [
						'-g',
						'-std=c++11',
						'-stdlib=libc++',
						'-pedantic',
						'-Wall',
						'-Wextra',
						'-Wno-unused-parameter',
					],
				}],
			],
			'xcode_settings': {
				'ALWAYS_SEARCH_USER_PATHS': 'NO',
				'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
				'CLANG_CXX_LIBRARY': 'libc++',
				'WARNING_CFLAGS': [
					'-Wall',
					'-Wextra',
					'-Wno-unused-parameter',
				],
			},
		},
	]
}
