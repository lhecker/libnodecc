# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'boringssl',
      'type': 'static_library',
      'includes': [
        'boringssl.gypi',
      ],
      'sources': [
        '<@(boringssl_lib_sources)',
      ],
      'defines': [ 'BORINGSSL_IMPLEMENTATION' ],
      'conditions': [
        ['target_arch == "arm"', {
          'conditions': [
            # 2014, Leonard Hecker -- added OPENSSL_NO_ASM and
            # removed assembler sources to solve compatibility issues with iOS
            # https://code.google.com/p/webrtc/issues/detail?id=3605
            # https://code.google.com/p/chromium/issues/detail?id=338886
            # https://code.google.com/p/chromium/issues/detail?id=410532
            # TODO: remove this once this is fixed
            ['OS == "mac"', {
              'defines': [ 'OPENSSL_NO_ASM' ],
            }, {
              'sources': [ '<@(boringssl_linux_arm_sources)' ],
            }],
          ],
        }],
        ['target_arch == "ia32"', {
          'conditions': [
            ['OS == "mac"', {
              'sources': [ '<@(boringssl_mac_x86_sources)' ],
            }],
            ['OS == "linux" or OS == "android"', {
              'sources': [ '<@(boringssl_linux_x86_sources)' ],
            }],
            ['OS != "mac" and OS != "linux" and OS != "android"', {
              'defines': [ 'OPENSSL_NO_ASM' ],
            }],
          ]
        }],
        ['target_arch == "x64"', {
          'conditions': [
            ['OS == "mac"', {
              'sources': [ '<@(boringssl_mac_x86_64_sources)' ],
            }],
            ['OS == "linux" or OS == "android"', {
              'sources': [ '<@(boringssl_linux_x86_64_sources)' ],
            }],
            ['OS == "win"', {
              'sources': [ '<@(boringssl_win_x86_64_sources)' ],
            }],
            ['OS != "mac" and OS != "linux" and OS != "win" and OS != "android"', {
              'defines': [ 'OPENSSL_NO_ASM' ],
            }],
          ]
        }],
        ['target_arch != "arm" and target_arch != "ia32" and target_arch != "x64"', {
          'defines': [ 'OPENSSL_NO_ASM' ],
        }],
      ],
      'include_dirs': [
        'boringssl/include',
        # This is for arm_arch.h, which is needed by some asm files. Since the
        # asm files are generated and kept in a different directory, they
        # cannot use relative paths to find this file.
        'boringssl/crypto',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'boringssl/include',
        ],
      },
    },
  ],
}
