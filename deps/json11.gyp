{
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
