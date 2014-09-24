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
					'-O0',
				],
				'msvs_settings': {
					'VCCLCompilerTool': {
						'target_conditions': [
							['uv_library=="static_library"', {
								'RuntimeLibrary': 1,
							}, {
								'RuntimeLibrary': 3,
							}],
						],
						'Optimization': 0,
						'MinimalRebuild': 'false',
						'OmitFramePointers': 'false',
						'BasicRuntimeChecks': 3,
					},
					'VCLinkerTool': {
						'LinkIncremental': 2,
					},
				},
				'xcode_settings': {
					'GCC_OPTIMIZATION_LEVEL': '0',
					'ONLY_ACTIVE_ARCH': 'YES',
				},
			},
			'Release': {
				'defines': [
					'NDEBUG',
				],
				'cflags': [
					'-O3',
				],
				'msvs_settings': {
					'VCCLCompilerTool': {
						'target_conditions': [
							['uv_library=="static_library"', {
								'RuntimeLibrary': 0,
							}, {
								'RuntimeLibrary': 2,
							}],
						],
						'Optimization': 3,
						'WholeProgramOptimization': 'true',
					},
					'VCLibrarianTool': {
						'AdditionalOptions': [
							'/LTCG',
						],
					},
					'VCLinkerTool': {
						'LinkTimeCodeGeneration': 1,
						'OptimizeReferences': 2,
						'EnableCOMDATFolding': 2,
						'LinkIncremental': 1,
					},
				},
				'xcode_settings': {
					'GCC_OPTIMIZATION_LEVEL': '3',
					'ONLY_ACTIVE_ARCH': 'NO',
				},
			}
		},
		'msvs_settings': {
			'VCCLCompilerTool': {
				'StringPooling': 'true',
				'DebugInformationFormat': 3,
				'WarningLevel': 3,
				'BufferSecurityCheck': 'true',
				'SuppressStartupBanner': 'true',
				'WarnAsError': 'false',
				'AdditionalOptions': [
					'/MP',
				 ],
			},
			'VCLibrarianTool': {
			},
			'VCLinkerTool': {
				'GenerateDebugInformation': 'true',
				'RandomizedBaseAddress': 2,
				'DataExecutionPrevention': 2,
				'AllowIsolation': 'true',
				'SuppressStartupBanner': 'true',
			},
		},
		'cflags': [
			'-Wall',
			'-Wextra',
			'-Wno-unused-parameter',
			'-fno-common',
		],
		'xcode_settings': {
			'ALWAYS_SEARCH_USER_PATHS': 'NO',
			'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
			'GCC_NO_COMMON_BLOCKS': 'YES',
			'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
			'USE_HEADERMAP': 'NO',
			'WARNING_CFLAGS': [
				'-Wall',
				'-Wextra',
				'-Wno-unused-parameter',
			],
			'target_conditions': [
				['uv_library=="static_library"', {
					'GCC_DYNAMIC_NO_PIC': 'YES',
				}, {
					'GCC_DYNAMIC_NO_PIC': 'NO',
				}],
			],
		},
	},
}
