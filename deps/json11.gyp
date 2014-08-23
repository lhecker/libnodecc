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
					'-std=c++11',
					'-stdlib=libc++',
					'-g',
					'-O0',
				],
				'xcode_settings': {
					'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
					'CLANG_CXX_LIBRARY': 'libc++',
					'GCC_OPTIMIZATION_LEVEL': '0',
				},
				'msvs_settings': {
					'VCCLCompilerTool': {
						'RuntimeLibrary': 3, # DLL debug
						'Optimization': 0, # /Od, no optimization
						'MinimalRebuild': 'false',
						'OmitFramePointers': 'false',
						'BasicRuntimeChecks': 3, # /RTC1
					},
					'VCLinkerTool': {
						'LinkIncremental': 2, # enable incremental linking
					},
				},
			},
			'Release': {
				'defines': [
					'NDEBUG',
				],
				'cflags': [
					'-std=c++11',
					'-stdlib=libc++',
					'-O3',
				],
				'xcode_settings': {
					'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
					'CLANG_CXX_LIBRARY': 'libc++',
					'GCC_OPTIMIZATION_LEVEL': '3',
				},
				'msvs_settings': {
					'VCCLCompilerTool': {
						'RuntimeLibrary': 2, # debug release
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
	},

	'targets': [
		{
			'target_name': 'json11',
			'type': 'static_library',
			'include_dirs': [ '.' ],
			'sources': [
				'json11/json11.cpp',
				'json11/json11.hpp',
			],
			'direct_dependent_settings': {
				'include_dirs': [ '.' ],
			},
		},
	],
}
