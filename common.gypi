{
	'variables': {
		'uv_library%': '<(library)',
	},

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
							['library=="static_library"', {
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
							['library=="static_library"', {
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
		'xcode_settings': {
			'COMBINE_HIDPI_IMAGES': 'YES',
			'DEAD_CODE_STRIPPING': 'YES',
			'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
			'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
			'WARNING_CFLAGS': [
				'-Wall',
				'-Wextra',
				'-Wno-unused-parameter',
			],
		},
		'cflags': [
			'-fdata-sections',
			'-ffunction-sections',
			'-fno-common',
			'-std=c++14',
			'-stdlib=libc++',
			'-Wall',
			'-Wextra',
			'-Wno-unused-parameter',
		],
		'ldflags': [
			'-Wl,--gc-sections',
		],
	},

	# Some Xcode settings need to be set project-wide instead
	# being target specific to suppress certain warnings.
	# Except for some specific bits (like CODE_SIGN_IDENTITY or TARGETED_DEVICE_FAMILY)
	# this recreates the default setting of projects created in Xcode 7.
	'xcode_settings': {
		'ALWAYS_SEARCH_USER_PATHS': 'NO',
		'CLANG_CXX_LANGUAGE_STANDARD': 'c++14',
		'CLANG_CXX_LIBRARY': 'libc++',
		'CLANG_ENABLE_MODULES': 'YES',
		'CLANG_ENABLE_OBJC_ARC': 'YES',
		'CLANG_WARN__DUPLICATE_METHOD_MATCH': 'YES',
		'CLANG_WARN_BOOL_CONVERSION': 'YES',
		'CLANG_WARN_CONSTANT_CONVERSION': 'YES',
		'CLANG_WARN_DIRECT_OBJC_ISA_USAGE': 'YES_ERROR',
		'CLANG_WARN_EMPTY_BODY': 'YES',
		'CLANG_WARN_ENUM_CONVERSION': 'YES',
		'CLANG_WARN_INT_CONVERSION': 'YES',
		'CLANG_WARN_OBJC_ROOT_CLASS': 'YES_ERROR',
		'CLANG_WARN_UNREACHABLE_CODE': 'YES',
		'COPY_PHASE_STRIP': 'NO',
		'ENABLE_STRICT_OBJC_MSGSEND': 'YES',
		'GCC_C_LANGUAGE_STANDARD': 'gnu99',
		'GCC_DYNAMIC_NO_PIC': 'NO',
		'GCC_NO_COMMON_BLOCKS': 'YES',
		'GCC_WARN_64_TO_32_BIT_CONVERSION': 'YES',
		'GCC_WARN_ABOUT_RETURN_TYPE': 'YES_ERROR',
		'GCC_WARN_UNDECLARED_SELECTOR': 'YES',
		'GCC_WARN_UNINITIALIZED_AUTOS': 'YES_AGGRESSIVE',
		'GCC_WARN_UNUSED_FUNCTION': 'YES',
		'GCC_WARN_UNUSED_VARIABLE': 'YES',

		'conditions': [
			['target_arch=="arm"', {
				'SDKROOT': 'iphoneos',
			}, {
				'SDKROOT': 'macosx',
			}],
		],
	},
	'configurations': {
		'Debug': {
			'xcode_settings': {
				'DEBUG_INFORMATION_FORMAT': 'dwarf',
				'ENABLE_TESTABILITY': 'YES',
				'GCC_OPTIMIZATION_LEVEL': '0',
				'GCC_PREPROCESSOR_DEFINITIONS': 'DEBUG=1',
				'MTL_ENABLE_DEBUG_INFO': 'YES',
				'ONLY_ACTIVE_ARCH': 'YES',
			},
		},
		'Release': {
			'xcode_settings': {
				'DEBUG_INFORMATION_FORMAT': 'dwarf-with-dsym',
				'ENABLE_NS_ASSERTIONS': 'NO',
				'MTL_ENABLE_DEBUG_INFO': 'NO',
				'VALIDATE_PRODUCT': 'YES',
			},
		},
	},
}
