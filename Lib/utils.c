#include "utils.h"

// user-defined constant
const char charset[CHARSET_SIZE] = "0123456789,."
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// generated from charset above using ReduceGenerator
const char reductionMap[REDUCTION_TABLE_SIZE] =
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

void reduce(password_t out, hash_t const in, size_t length, uint32_t salt)
{
    unsigned int i;

    for (i = 0; i < (length + 2) / 3; ++i) {
        uint32_t word = in[i] + salt;
        unsigned int j;

        for (j = 0; j < 3 && 3 * i + j < length; ++j) {
            uint32_t index = word % REDUCTION_TABLE_SIZE;

            out[3 * i + j] = reductionMap[index];
            word /= REDUCTION_TABLE_SIZE;
        }
    }
}

void printHash(char *out, const hash_t hash)
{
    int i;

    for (i = 0; i < 4; ++i) {
        sprintf(&out[8 * i], "%02x%02x%02x%02x",
                hash[i] & 0xff,
                (hash[i] >> 8) & 0xff,
                (hash[i] >> 16) & 0xff,
                (hash[i] >> 24) & 0xff);
    }
}
