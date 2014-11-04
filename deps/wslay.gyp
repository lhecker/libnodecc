{
	'targets': [
		{
			'target_name': 'wslay',
			'type': 'static_library',
			'include_dirs': [ 'wslay/lib/includes' ],
			'defines': [
				'WSLAY_VERSION=0.1.1',
			],
			'sources': [
				'wslay/lib/wslay_event.c',
				'wslay/lib/wslay_frame.c',
				'wslay/lib/wslay_net.c',
				'wslay/lib/wslay_queue.c',
				'wslay/lib/wslay_stack.c',
			],
			'direct_dependent_settings': {
				'include_dirs': [ 'wslay/lib/includes' ],
				'defines': [
					'WSLAY_VERSION=0.1.1',
				],
			},
		},
	],
}
