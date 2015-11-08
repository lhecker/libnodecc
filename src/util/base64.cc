#include "libnodecc/util/base64.h"

#include "libnodecc/util/endian.h"


static const uint8_t decode_table[256] = {
	255 /* NUL */, 255 /* SOH */, 255 /* STX */, 255 /* ETX */, 255 /* EOT */, 255 /* ENQ */, 255 /* ACK */, 255 /* BEL */, 255 /* BS  */, 255 /* HT  */, 255 /* LF  */, 255 /* VT  */, 255 /* FF  */, 255 /* CR  */, 255 /* SO  */, 255 /* SI  */,
	255 /* DLE */, 255 /* DC1 */, 255 /* DC2 */, 255 /* DC3 */, 255 /* DC4 */, 255 /* NAK */, 255 /* SYN */, 255 /* ETB */, 255 /* CAN */, 255 /* EM  */, 255 /* SUB */, 255 /* ESC */, 255 /* FS  */, 255 /* GS  */, 255 /* RS  */, 255 /* US  */,
	255 /* SP  */, 255 /* !   */, 255 /* "   */, 255 /* #   */, 255 /* $   */, 255 /* %   */, 255 /* &   */, 255 /* '   */, 255 /* (   */, 255 /* )   */, 255 /* *   */, 62  /* +   */, 255 /* ,   */, 62  /* -   */, 255 /* .   */, 63  /* /   */,
	52  /* 0   */, 53  /* 1   */, 54  /* 2   */, 55  /* 3   */, 56  /* 4   */, 57  /* 5   */, 58  /* 6   */, 59  /* 7   */, 60  /* 8   */, 61  /* 9   */, 255 /* :   */, 255 /* ;   */, 255 /* <   */, 255 /* =   */, 255 /* >   */, 255 /* ?   */,
	255 /* @   */, 0   /* A   */, 1   /* B   */, 2   /* C   */, 3   /* D   */, 4   /* E   */, 5   /* F   */, 6   /* G   */, 7   /* H   */, 8   /* I   */, 9   /* J   */, 10  /* K   */, 11  /* L   */, 12  /* M   */, 13  /* N   */, 14  /* O   */,
	15  /* P   */, 16  /* Q   */, 17  /* R   */, 18  /* S   */, 19  /* T   */, 20  /* U   */, 21  /* V   */, 22  /* W   */, 23  /* X   */, 24  /* Y   */, 25  /* Z   */, 255 /* [   */, 255 /* \   */, 255 /* ]   */, 255 /* ^   */, 63  /* _   */,
	255 /* `   */, 26  /* a   */, 27  /* b   */, 28  /* c   */, 29  /* d   */, 30  /* e   */, 31  /* f   */, 32  /* g   */, 33  /* h   */, 34  /* i   */, 35  /* j   */, 36  /* k   */, 37  /* l   */, 38  /* m   */, 39  /* n   */, 40  /* o   */,
	41  /* p   */, 42  /* q   */, 43  /* r   */, 44  /* s   */, 45  /* t   */, 46  /* u   */, 47  /* v   */, 48  /* w   */, 49  /* x   */, 50  /* y   */, 51  /* z   */, 255 /* {   */, 255 /* |   */, 255 /* }   */, 255 /* ~   */, 255 /* DEL */,
};

// the def(ault) base64 encoding
static const uint8_t encode_table_def[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

// the special base64url encoding
static const uint8_t encode_table_url[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_' };


namespace node {
namespace util {

node::buffer base64::encode(const node::buffer_view& buffer, bool base64url) {
	const uint8_t* encode_table = base64url ? encode_table_url : encode_table_def;

	uint8_t* base = (uint8_t*)buffer.data();
	const size_t len = buffer.size();
	const uint8_t* end = base + len;

	node::mutable_buffer result;
	result.set_size(((buffer.size() + 2) / 3) * 4);

	if (result) {
		uint8_t *out = result.data<uint8_t>();

		if (len >= 12) {
#if 0
			// OBVIOUS ENCODE (bytewise iteration)
			// TODO: complete this version by wrapping it in an actual loop

			const uint8_t in0  = base[0];
			const uint8_t in1  = base[1];
			const uint8_t in2  = base[2];
			const uint8_t in3  = base[3];
			const uint8_t in4  = base[4];
			const uint8_t in5  = base[5];
			const uint8_t in6  = base[6];
			const uint8_t in7  = base[7];
			const uint8_t in8  = base[8];
			const uint8_t in9  = base[9];
			const uint8_t in10 = base[10];
			const uint8_t in11 = base[11];


			out[0]  = encode_table[in0 >> 2];
			out[1]  = encode_table[(in0 & 0x03) << 4 | in1 >> 4];
			out[2]  = encode_table[(in1 & 0x0f) << 2 | in2 >> 6];
			out[3]  = encode_table[in2 & 0x3f];

			out[4]  = encode_table[(in3 & 0xfc) >> 2];
			out[5]  = encode_table[(in3 & 0x03) << 4 | in4 >> 4];
			out[6]  = encode_table[(in4 & 0x0f) << 2 | in5 >> 6];
			out[7]  = encode_table[in5 & 0x3f];

			out[8]  = encode_table[(in6 & 0xfc) >> 2];
			out[9]  = encode_table[(in6 & 0x03) << 4 | in7 >> 4];
			out[10] = encode_table[(in7 & 0x0f) << 2 | in8 >> 6];
			out[11] = encode_table[in8 & 0x3f];

			out[12] = encode_table[(in9 & 0xfc) >> 2];
			out[13] = encode_table[(in9 & 0x03) << 4 | in10 >> 4];
			out[14] = encode_table[(in10 & 0x0f) << 2 | in11 >> 6];
			out[15] = encode_table[in11 & 0x3f];

			base += 12;
			out += 16;

#else
			// FAST ENCODE (3 words per iteration)
			// TODO: write an unit test to show it's superiority

			// check if base is properly aligned to uint32_t
			uint8_t offset = uintptr_t(base) & 3;

			if (offset) {
				/*
				 * The Base64 conversion maps 3 Bytes from the input
				 * to 4 Bytes in the output.
				 *
				 * This means the aligned pointer y from the
				 * original pointer x needs to fulfill following requirements:
				 *   1. y >= x
				 *   2. y = 0 (mod 4)      // y should be aligned to uint32_t
				 *   3. y - x = 0 (mod 3)  // 3 input bytes align to 4 *whole* bytes in the output
				 *
				 *  I  x = 4a + b   // every pointer x can be described with this equation
				 * II  y = x + 3b
				 *
				 * y - x = 3b
				 * --> 3. requirement fulfilled
				 *
				 * insert I in II
				 * y = 4a + b + 3b
				 * y = 4a + 4b
				 * y = 4(a + b)
				 * --> 2. requirement fulfilled
				 */
				const uint8_t* end_unaligned = (uint8_t*)(uintptr_t(base) + 3 * offset);

				while (base < end_unaligned) {
					const uint8_t in0 = base[0];
					const uint8_t in1 = base[1];
					const uint8_t in2 = base[2];

					out[0] = encode_table[in0 >> 2];
					out[1] = encode_table[(in0 & 0x03) << 4 | in1 >> 4];
					out[2] = encode_table[(in1 & 0x0f) << 2 | in2 >> 6];
					out[3] = encode_table[in2 & 0x3f];

					base += 3;
					out += 4;
				}
			}

			const uint8_t* end_aligned = base + len - 11;

			while (base < end_aligned) {
				/*
				 * This code first converts the input bytes to big endian.
				 * Benchmarking suggested that this is about 15-20% faster on x86 and ARM,
				 * since less bit juggling is needed afterwards to extract the 16 sextets from the 3 words.
				 * (which both are "first-class" deployment architectures and have a builtin "bswap" instruction).
				 *
				 * For possible future benchmarks the "obvious" approach is still included above.
				 * (Search for "OBVIOUS ENCODE".)
				 */
				const uint32_t* in = (uint32_t*)((void*)base);
				const uint32_t in1 = htonl(in[0]);
				const uint32_t in2 = htonl(in[1]);
				const uint32_t in3 = htonl(in[2]);

				out[0]  = encode_table[in1 >> 26];
				out[1]  = encode_table[(in1 & 0x3F00000) >> 20];
				out[2]  = encode_table[(in1 & 0xFC000) >> 14];
				out[3]  = encode_table[(in1 & 0x3F00) >> 8];
				out[4]  = encode_table[(in1 & 0xFC) >> 2];

				out[5]  = encode_table[(in1 & 0x3) << 4 | in2 >> 28];

				out[6]  = encode_table[(in2 & 0xFC00000) >> 22];
				out[7]  = encode_table[(in2 & 0x3F0000) >> 16];
				out[8]  = encode_table[(in2 & 0xFC00) >> 10];
				out[9]  = encode_table[(in2 & 0x3F0) >> 4];

				out[10] = encode_table[(in2 & 0xF) << 2 | in3 >> 30];

				out[11] = encode_table[(in3 & 0x3F000000) >> 24];
				out[12] = encode_table[(in3 & 0xFC0000) >> 18];
				out[13] = encode_table[(in3 & 0x3F000) >> 12];
				out[14] = encode_table[(in3 & 0xFC0) >> 6];

				out[15] = encode_table[in3 & 0x3F];

				base += 12;
				out += 16;
			}

#endif
		}

		const uint8_t* end_unaligned = end - 2;

		while (base < end_unaligned) {
			const uint8_t in0 = base[0];
			const uint8_t in1 = base[1];
			const uint8_t in2 = base[2];

			out[0] = encode_table[in0 >> 2];
			out[1] = encode_table[(in0 & 0x03) << 4 | in1 >> 4];
			out[2] = encode_table[(in1 & 0x0f) << 2 | in2 >> 6];
			out[3] = encode_table[in2 & 0x3f];

			base += 3;
			out += 4;
		}

		const uint8_t left = end - base;

		uint8_t in0;
		uint8_t in1 = 0;
		uint8_t in2 = 0;

		switch (left) {
		case 3:
			in2 = base[2];
			out[3] = encode_table[in2 & 0x3f];
		case 2:
			in1 = base[1];
			out[2] = encode_table[(in1 & 0x0f) << 2 | in2 >> 6];
		case 1:
			in0 = base[0];
			out[1] = encode_table[(in0 & 0x03) << 4 | in1 >> 4];
			out[0] = encode_table[in0 >> 2];

			switch (left) {
			case 1:
				out[2] = '=';
			case 2:
				out[3] = '=';
			}
		}
	}

	return result;
}

node::buffer base64::decode(const node::buffer_view& buffer) {
#define shift6(n)       \
    if (n <= 0x3F) {    \
        r = r << 6 | n; \
        ri++;           \
    }

	uint8_t* base = (uint8_t*)buffer.data();
	const size_t len = buffer.size();
	const uint8_t* end = base + len;

	node::mutable_buffer result;
	result.set_size((len / 4) * 3);

	if (result) {
		uint8_t *out = result.data<uint8_t>();

		uint32_t r = 0;
		uint8_t ri = 0;

		const uint8_t* end_aligned = end - 3;

		while (base < end_aligned) {
			uint8_t a = decode_table[base[0]];
			uint8_t b = decode_table[base[1]];
			uint8_t c = decode_table[base[2]];
			uint8_t d = decode_table[base[3]];

			shift6(a)
			shift6(b)
			shift6(c)
			shift6(d)

			// TODO: add support for big endian systems by converting the constants
			switch (ri) {
			case 2:
				out[0] = uint8_t((r & 0x00000ff0ul) >>  4);
				out += 1;
				ri = 1;
				break;
			case 3:
				out[0] = uint8_t((r & 0x0003fc00ul) >> 10);
				out[1] = uint8_t((r & 0x000003fcul) >>  2);
				out += 2;
				ri = 1;
				break;
			case 4:
				out[0] = uint8_t((r & 0x00ff0000ul) >> 16);
				out[1] = uint8_t((r & 0x0000ff00ul) >>  8);
				out[2] = uint8_t((r & 0x000000fful) >>  0);
				out += 3;
				ri = 0;
				break;
			}

			base += 4;
		}

		const uint8_t left = end - base;

		for (uint8_t i = 0; i < left; i++) {
			uint8_t n = decode_table[base[i]];
			shift6(n)
		}

		switch (ri) {
		case 2:
			out[0] = uint8_t((r & 0x00000ff0ul) >>  4);
			break;
		case 3:
			out[0] = uint8_t((r & 0x0003fc00ul) >> 10);
			out[1] = uint8_t((r & 0x000003fcul) >>  2);
			break;
		case 4:
			out[0] = uint8_t((r & 0x00ff0000ul) >> 16);
			out[1] = uint8_t((r & 0x0000ff00ul) >>  8);
			out[2] = uint8_t((r & 0x000000fful) >>  0);
			break;
		}

		result.set_size(out - result.data<uint8_t>());

#undef shift6
	}

	return result;
}

} // namespace util
} // namespace node
