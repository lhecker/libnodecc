{
	'target_defaults': {
		'default_configuration': 'Debug',
		'configurations': {
			'Debug': {
				'defines': [
					'DEBUG',
					'_DEBUG',
				],
				'cflags': [
					'-g',
					'-O0',
				],
				'msvs_settings': {
					'VCCLCompilerTool': {
						'target_conditions': [
							['uv_library=="static_library"', {
								'RuntimeLibrary': 1, # static debug
							}, {
								'RuntimeLibrary': 3, # DLL debug
							}],
						],
						'Optimization': 0, # /Od, no optimization
						'MinimalRebuild': 'false',
						'OmitFramePointers': 'false',
						'BasicRuntimeChecks': 3, # /RTC1
					},
					'VCLinkerTool': {
						'LinkIncremental': 2, # enable incremental linking
					},
					'xcode_settings': {
						'GCC_OPTIMIZATION_LEVEL': '0',
						'OTHER_CFLAGS': [ '-Wno-strict-aliasing' ],
					},
				},
			},
			'Release': {
				'defines': [
					'NDEBUG',
				],
				'cflags': [
					'-O3',
					'-fstrict-aliasing',
					'-fomit-frame-pointer',
					'-fdata-sections',
					'-ffunction-sections',
				],
				'msvs_settings': {
					'VCCLCompilerTool': {
						'target_conditions': [
							['uv_library=="static_library"', {
								'RuntimeLibrary': 0, # static release
							}, {
								'RuntimeLibrary': 2, # debug release
							}],
						],
						'Optimization': 3, # /Ox, full optimization
						'FavorSizeOrSpeed': 1, # /Ot, favour speed over size
						'InlineFunctionExpansion': 2, # /Ob2, inline anything eligible
						'WholeProgramOptimization': 'true', # /GL, whole program optimization, needed for LTCG
						'OmitFramePointers': 'true',
						'EnableFunctionLevelLinking': 'true',
						'EnableIntrinsicFunctions': 'true',
					},
					'VCLibrarianTool': {
						'AdditionalOptions': [
							'/LTCG', # link time code generation
						],
					},
					'VCLinkerTool': {
						'LinkTimeCodeGeneration': 1, # link-time code generation
						'OptimizeReferences': 2, # /OPT:REF
						'EnableCOMDATFolding': 2, # /OPT:ICF
						'LinkIncremental': 1, # disable incremental linking
					},
				},
			},
		},
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
