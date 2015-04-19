// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  29 May 2007 - Initial Revision
//  25 Feb 2008 - moved to namespace boost::uuids::detail
//  10 Jan 2012 - can now handle the full size of messages (2^64 - 1 bits)
//  17 Apr 2015 - code cleanup and new get_digest_hex() method (by Leonard Hecker)

#ifndef nodecc_util_sha1_h
#define nodecc_util_sha1_h


#include "../buffer.h"


#define SHA1_DIGEST_LENGTH 20


namespace node {
namespace util {

class sha1 {
public:
	typedef uint8_t(&digest_t)[SHA1_DIGEST_LENGTH];

	explicit sha1();

	void reset();

	void push(uint8_t byte);
	void push(node::buffer_view buffer);

	void get_digest(digest_t digest);

private:
	void push_impl(uint8_t byte);
	void process_block();

	uint64_t _bit_count;
	uint32_t _h[5];
	uint8_t _block[64];
	std::size_t _block_byte_index;
};

} // namespace util
} // namespace node

#endif // nodecc_util_sha1_h
