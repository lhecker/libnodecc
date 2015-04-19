// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  29 May 2007 - Initial Revision
//  25 Feb 2008 - moved to namespace boost::uuids::detail
//  10 Jan 2012 - can now handle the full size of messages (2^64 - 1 bits)
//  17 Apr 2015 - added get_digest_hex() method & integrated into libnodecc (by Leonard Hecker)

#include "libnodecc/util/sha1.h"


static inline uint32_t left_rotate(uint32_t x, std::size_t n) {
	return (x << n) ^ (x >> (32 - n));
}


namespace node {
namespace util {

sha1::sha1() {
	this->_h[0] = 0x67452301;
	this->_h[1] = 0xefcdab89;
	this->_h[2] = 0x98badcfe;
	this->_h[3] = 0x10325476;
	this->_h[4] = 0xc3d2e1f0;

	this->_block_byte_index = 0;
	this->_bit_count = 0;
}

void sha1::push(uint8_t byte) {
	this->push_impl(byte);
	this->_bit_count += 8;
}

void sha1::push_impl(uint8_t byte) {
	this->_block[this->_block_byte_index++] = byte;

	if (this->_block_byte_index == 64) {
		this->_block_byte_index = 0;
		this->process_block();
	}
}

void sha1::push(node::buffer_view buffer) {
	const uint8_t* beg = buffer.data<const uint8_t>();
	const uint8_t* end = beg + buffer.size();

	for(; beg != end; beg++) {
		this->push(*beg);
	}
}

void sha1::process_block() {
	uint32_t w[80];

	for (std::size_t i = 0; i < 16; i++) {
		w[i]  = (this->_block[i * 4 + 0] << 24);
		w[i] |= (this->_block[i * 4 + 1] << 16);
		w[i] |= (this->_block[i * 4 + 2] <<  8);
		w[i] |= (this->_block[i * 4 + 3] <<  0);
	}

	for (std::size_t i = 16; i < 80; i++) {
		w[i] = left_rotate((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
	}

	uint32_t a = this->_h[0];
	uint32_t b = this->_h[1];
	uint32_t c = this->_h[2];
	uint32_t d = this->_h[3];
	uint32_t e = this->_h[4];

	for (std::size_t i = 0; i < 80; i++) {
		uint32_t f;
		uint32_t k;

		if (i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5a827999;
		} else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ed9eba1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8f1bbcdc;
		} else {
			f = b ^ c ^ d;
			k = 0xca62c1d6;
		}

		const uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = left_rotate(b, 30);
		b = a;
		a = temp;
	}

	this->_h[0] += a;
	this->_h[1] += b;
	this->_h[2] += c;
	this->_h[3] += d;
	this->_h[4] += e;
}

void sha1::get_digest(digest_t digest) {
	// append the bit '1' to the message
	this->push_impl(0x80);

	// append k bits '0', where k is the minimum number >= 0
	// such that the resulting message length is congruent to 56 (mod 64)
	// check if there is enough space for padding and bit_count
	if (this->_block_byte_index > 56) {
		// finish this block
		while (this->_block_byte_index != 0) {
			this->push_impl(0);
		}

		// one more block
		while (this->_block_byte_index < 56) {
			this->push_impl(0);
		}
	} else {
		while (this->_block_byte_index < 56) {
			this->push_impl(0);
		}
	}

	// append length of message (before pre-processing)
	// as a 64-bit big-endian integer
	this->push_impl(static_cast<uint8_t>((this->_bit_count >> 56) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >> 48) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >> 40) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >> 32) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >> 24) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >> 16) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >>  8) & 0xff));
	this->push_impl(static_cast<uint8_t>((this->_bit_count >>  0) & 0xff));

	for (size_t i = 0; i < 5; i++) {
		const uint32_t h = this->_h[i];
		digest[i * 4 + 0] = (h >> 12) & 0xff000000;
		digest[i * 4 + 1] = (h >>  8) & 0x00ff0000;
		digest[i * 4 + 2] = (h >>  4) & 0x0000ff00;
		digest[i * 4 + 3] = (h >>  0) & 0x000000ff;
	}
}

} // namespace util
} // namespace node
