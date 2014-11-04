# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is created by update_gypi_and_asm.py. Do not edit manually.

{
  'targets': [
    {
      'target_name': 'boringssl_base64_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/base64/base64_test.c',
      ],
    },
    {
      'target_name': 'boringssl_bio_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/bio/bio_test.c',
      ],
    },
    {
      'target_name': 'boringssl_bn_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/bn/bn_test.c',
      ],
    },
    {
      'target_name': 'boringssl_bytestring_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/bytestring/bytestring_test.c',
      ],
    },
    {
      'target_name': 'boringssl_aead_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/cipher/aead_test.c',
      ],
    },
    {
      'target_name': 'boringssl_cipher_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/cipher/cipher_test.c',
      ],
    },
    {
      'target_name': 'boringssl_dh_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/dh/dh_test.c',
      ],
    },
    {
      'target_name': 'boringssl_dsa_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/dsa/dsa_test.c',
      ],
    },
    {
      'target_name': 'boringssl_example_mul',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/ec/example_mul.c',
      ],
    },
    {
      'target_name': 'boringssl_ecdsa_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/ecdsa/ecdsa_test.c',
      ],
    },
    {
      'target_name': 'boringssl_err_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/err/err_test.c',
      ],
    },
    {
      'target_name': 'boringssl_example_sign',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/evp/example_sign.c',
      ],
    },
    {
      'target_name': 'boringssl_hmac_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/hmac/hmac_test.c',
      ],
    },
    {
      'target_name': 'boringssl_lhash_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/lhash/lhash_test.c',
      ],
    },
    {
      'target_name': 'boringssl_md5_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/md5/md5_test.c',
      ],
    },
    {
      'target_name': 'boringssl_gcm_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/modes/gcm_test.c',
      ],
    },
    {
      'target_name': 'boringssl_pkcs12_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/pkcs8/pkcs12_test.c',
      ],
    },
    {
      'target_name': 'boringssl_rsa_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/rsa/rsa_test.c',
      ],
    },
    {
      'target_name': 'boringssl_sha1_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/sha/sha1_test.c',
      ],
    },
    {
      'target_name': 'boringssl_pkcs7_test',
      'type': 'executable',
      'dependencies': [
        'boringssl.gyp:boringssl',
      ],
      'sources': [
        'boringssl/crypto/x509/pkcs7_test.c',
      ],
    },
  ],
  'variables': {
    'boringssl_test_targets': [
      'boringssl_aead_test',
      'boringssl_base64_test',
      'boringssl_bio_test',
      'boringssl_bn_test',
      'boringssl_bytestring_test',
      'boringssl_cipher_test',
      'boringssl_dh_test',
      'boringssl_dsa_test',
      'boringssl_ecdsa_test',
      'boringssl_err_test',
      'boringssl_example_mul',
      'boringssl_example_sign',
      'boringssl_gcm_test',
      'boringssl_hmac_test',
      'boringssl_lhash_test',
      'boringssl_md5_test',
      'boringssl_pkcs12_test',
      'boringssl_pkcs7_test',
      'boringssl_rsa_test',
      'boringssl_sha1_test',
    ],
  }
}
