
#include "Lib/config.h"

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : disable

// Macro for writing chars to 32-bit integers
#define PUTCHAR(buf, index, val) (buf)[(index)>>2] = \
    ((buf)[(index)>>2] & ~(0xffU << (((index) & 3) << 3))) \
    + ((val) << (((index) & 3) << 3))

// The basic MD5 functions
#define F(x, y, z)			((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)			((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)			((x) ^ (y) ^ (z))
#define I(x, y, z)			((y) ^ ((x) | ~(z)))

// The MD5 transformation for all four rounds.
#define STEP(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);

#ifdef USE_VECTORS
#define DATA_TYPE       uint4
#else
#define DATA_TYPE       uint
#endif

#define DATA_LOC

#define REDUCTION_TABLE_SIZE    512
__constant const char reductionMap[REDUCTION_TABLE_SIZE] =
                                    "bKeixL,OfX.IyFAoPVafpxZtjXBRzG7w"
                                    "JIDdQ8IKLzI6VG0n9j0b4i7xbcKBg640"
                                    "EqOT90SdpdMAehKBgdthFmQGanZe5Fq2"
                                    ".LEyuhrYAqcovBsKIezn4gFOTXhHX3Db"
                                    "J6TPsN2NGW0uhvYlSBy1pmvD,40nWXvx"
                                    "wmesYED7zd5yPTF73HcEsiWU5u8LfMwg"
                                    "65MgL63NdGWdO,P8OVx1.X4R5i6lcp.h"
                                    "NM4YiCVUMtrV,Ear1I3BySLUrzfQS,tz"
                                    "xO5qukVcCA1f1.ile9786JJQDy2JxQ.o"
                                    "loYVksvuocPNRNRvCE8tpq7tw4mOQJoA"
                                    "aYEaQAYqZXblV8k1a9uirCZMvyQJryFn"
                                    "DHA,9XFMo2Y0c,nsabwC7CDSS9MkwjzK"
                                    "CKibkDUT6.HZRuoh5PgUg3IpEGGZZ518"
                                    "rfNuHW,jpHjWlSR2ngUNtnO21z3te3HG"
                                    "3lKhLeUm9kPjsmxvTHjW9LZWjTRbw8fa"
                                    "kmJsB7f0IArk4cwFql.CBPTR2mdpU2qS";

inline void reduce(DATA_LOC DATA_TYPE *data, uint length, uint salt)
{
    uint i;

    for (i = 0; i < (length + 2) / 3; ++i) {
        DATA_TYPE word = data[i] + salt;
        uint j;

        data[i] = (DATA_TYPE)(0);

        for (j = 0; j < 3 && 3 * i + j < length; ++j) {
            DATA_TYPE index = word % REDUCTION_TABLE_SIZE;
#ifdef USE_VECTORS
            DATA_TYPE byte = (DATA_TYPE)(reductionMap[index.x],
                                 reductionMap[index.y],
                                 reductionMap[index.z],
                                 reductionMap[index.w]);
#else
            DATA_TYPE byte = reductionMap[index];
#endif
            PUTCHAR(data, 3 * i + j, byte);
            word /= REDUCTION_TABLE_SIZE;
        }
    }

    for (; i < 4; ++i)
        data[i] = (DATA_TYPE)(0);

    PUTCHAR(data, length, 0x80);
}

// calculates MD5 hashes of 4 input passwords at a time
// passwords must be terminated by 0x80, not zero
// maximum length is MAX_PASSWD including padding
inline void md5(DATA_LOC DATA_TYPE *keys, uint passwordLength)
{
    DATA_TYPE a = (DATA_TYPE)(0x67452301);
    DATA_TYPE b = (DATA_TYPE)(0xefcdab89);
    DATA_TYPE c = (DATA_TYPE)(0x98badcfe);
    DATA_TYPE d = (DATA_TYPE)(0x10325476);
    DATA_TYPE len = (DATA_TYPE)(8 * passwordLength);

    // Round 1
    STEP(F, a, b, c, d, keys[0],        0xd76aa478, 7)
    STEP(F, d, a, b, c, keys[1],        0xe8c7b756, 12)
    STEP(F, c, d, a, b, keys[2],        0x242070db, 17)
    STEP(F, b, c, d, a, keys[3],        0xc1bdceee, 22)
    STEP(F, a, b, c, d, (DATA_TYPE)(0), 0xf57c0faf, 7)
    STEP(F, d, a, b, c, (DATA_TYPE)(0), 0x4787c62a, 12)
    STEP(F, c, d, a, b, (DATA_TYPE)(0), 0xa8304613, 17)
    STEP(F, b, c, d, a, (DATA_TYPE)(0), 0xfd469501, 22)
    STEP(F, a, b, c, d, (DATA_TYPE)(0), 0x698098d8, 7)
    STEP(F, d, a, b, c, (DATA_TYPE)(0), 0x8b44f7af, 12)
    STEP(F, c, d, a, b, (DATA_TYPE)(0), 0xffff5bb1, 17)
    STEP(F, b, c, d, a, (DATA_TYPE)(0), 0x895cd7be, 22)
    STEP(F, a, b, c, d, (DATA_TYPE)(0), 0x6b901122, 7)
    STEP(F, d, a, b, c, (DATA_TYPE)(0), 0xfd987193, 12)
    STEP(F, c, d, a, b, len,            0xa679438e, 17)
    STEP(F, b, c, d, a, (DATA_TYPE)(0), 0x49b40821, 22)

    // Round 2
    STEP(G, a, b, c, d, keys[1],        0xf61e2562, 5)
    STEP(G, d, a, b, c, (DATA_TYPE)(0), 0xc040b340, 9)
    STEP(G, c, d, a, b, (DATA_TYPE)(0), 0x265e5a51, 14)
    STEP(G, b, c, d, a, keys[0],        0xe9b6c7aa, 20)
    STEP(G, a, b, c, d, (DATA_TYPE)(0), 0xd62f105d, 5)
    STEP(G, d, a, b, c, (DATA_TYPE)(0), 0x02441453, 9)
    STEP(G, c, d, a, b, (DATA_TYPE)(0), 0xd8a1e681, 14)
    STEP(G, b, c, d, a, (DATA_TYPE)(0), 0xe7d3fbc8, 20)
    STEP(G, a, b, c, d, (DATA_TYPE)(0), 0x21e1cde6, 5)
    STEP(G, d, a, b, c, len,            0xc33707d6, 9)
    STEP(G, c, d, a, b, keys[3],        0xf4d50d87, 14)
    STEP(G, b, c, d, a, (DATA_TYPE)(0), 0x455a14ed, 20)
    STEP(G, a, b, c, d, (DATA_TYPE)(0), 0xa9e3e905, 5)
    STEP(G, d, a, b, c, keys[2],        0xfcefa3f8, 9)
    STEP(G, c, d, a, b, (DATA_TYPE)(0), 0x676f02d9, 14)
    STEP(G, b, c, d, a, (DATA_TYPE)(0), 0x8d2a4c8a, 20)

    // Round 3
    STEP(H, a, b, c, d, (DATA_TYPE)(0), 0xfffa3942, 4)
    STEP(H, d, a, b, c, (DATA_TYPE)(0), 0x8771f681, 11)
    STEP(H, c, d, a, b, (DATA_TYPE)(0), 0x6d9d6122, 16)
    STEP(H, b, c, d, a, len,            0xfde5380c, 23)
    STEP(H, a, b, c, d, keys[1],        0xa4beea44, 4)
    STEP(H, d, a, b, c, (DATA_TYPE)(0), 0x4bdecfa9, 11)
    STEP(H, c, d, a, b, (DATA_TYPE)(0), 0xf6bb4b60, 16)
    STEP(H, b, c, d, a, (DATA_TYPE)(0), 0xbebfbc70, 23)
    STEP(H, a, b, c, d, (DATA_TYPE)(0), 0x289b7ec6, 4)
    STEP(H, d, a, b, c, keys[0],        0xeaa127fa, 11)
    STEP(H, c, d, a, b, keys[3],        0xd4ef3085, 16)
    STEP(H, b, c, d, a, (DATA_TYPE)(0), 0x04881d05, 23)
    STEP(H, a, b, c, d, (DATA_TYPE)(0), 0xd9d4d039, 4)
    STEP(H, d, a, b, c, (DATA_TYPE)(0), 0xe6db99e5, 11)
    STEP(H, c, d, a, b, (DATA_TYPE)(0), 0x1fa27cf8, 16)
    STEP(H, b, c, d, a, keys[2],        0xc4ac5665, 23)

    // Round 4
    STEP(I, a, b, c, d, keys[0],        0xf4292244, 6)
    STEP(I, d, a, b, c, (DATA_TYPE)(0), 0x432aff97, 10)
    STEP(I, c, d, a, b, len,            0xab9423a7, 15)
    STEP(I, b, c, d, a, (DATA_TYPE)(0), 0xfc93a039, 21)
    STEP(I, a, b, c, d, (DATA_TYPE)(0), 0x655b59c3, 6)
    STEP(I, d, a, b, c, keys[3],        0x8f0ccc92, 10)
    STEP(I, c, d, a, b, (DATA_TYPE)(0), 0xffeff47d, 15)
    STEP(I, b, c, d, a, keys[1],        0x85845dd1, 21)
    STEP(I, a, b, c, d, (DATA_TYPE)(0), 0x6fa87e4f, 6)
    STEP(I, d, a, b, c, (DATA_TYPE)(0), 0xfe2ce6e0, 10)
    STEP(I, c, d, a, b, (DATA_TYPE)(0), 0xa3014314, 15)
    STEP(I, b, c, d, a, (DATA_TYPE)(0), 0x4e0811a1, 21)
    STEP(I, a, b, c, d, (DATA_TYPE)(0), 0xf7537e82, 6)
    STEP(I, d, a, b, c, (DATA_TYPE)(0), 0xbd3af235, 10)
    STEP(I, c, d, a, b, keys[2],        0x2ad7d2bb, 15)
    STEP(I, b, c, d, a, (DATA_TYPE)(0), 0xeb86d391, 21)

    keys[0] = a + 0x67452301;
    keys[1] = b + 0xefcdab89;
    keys[2] = c + 0x98badcfe;
    keys[3] = d + 0x10325476;
}

struct args {
    uint chainsInBlock;
    uint chainLength;
    uint passwordLength;
    uint numberOfChains;
    uint showDist;
};

__kernel void rainbow(__global uint *hashes, __constant uint *passwords,
                      struct args args)
{
    uint id = get_global_id(0);
    DATA_LOC DATA_TYPE buf[MAX_PASSWD / 4];
    uint len = args.passwordLength;
    int i;

    for (i = 0; i < MAX_PASSWD / 4; ++i) {
#ifdef USE_VECTORS
        buf[i].x = passwords[(4 * id + 0) * (MAX_PASSWD / 4) + i];
        buf[i].y = passwords[(4 * id + 1) * (MAX_PASSWD / 4) + i];
        buf[i].z = passwords[(4 * id + 2) * (MAX_PASSWD / 4) + i];
        buf[i].w = passwords[(4 * id + 3) * (MAX_PASSWD / 4) + i];
#else
        buf[i] = passwords[id * (MAX_PASSWD / 4) + i];
#endif
    }

    PUTCHAR(buf, len, 0x80);

    for (i = 0; i < args.chainLength; ++i) {
        md5(buf, len);
        reduce(buf, len, i);
    }
    md5(buf, len);

    for (i = 0; i < 4; ++i) {
#ifdef USE_VECTORS
        hashes[(4 * id + 0) * 4 + i] = buf[i].x;
        hashes[(4 * id + 1) * 4 + i] = buf[i].y;
        hashes[(4 * id + 2) * 4 + i] = buf[i].z;
        hashes[(4 * id + 3) * 4 + i] = buf[i].w;
#else
        hashes[id * 4 + i] = buf[i];
#endif
    }
}
