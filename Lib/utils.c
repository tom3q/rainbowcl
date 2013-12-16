#include "utils.h"

const char charset[CHARSET_SIZE] = "0123456789,."
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

uint32_t charsetStats[CHARSET_SIZE] = { 0, };

void reduce(password_t out, hash_t const in, size_t length, uint32_t salt)
{
    uint8_t bits = 32 - __builtin_clz(CHARSET_SIZE - 1);
    uint8_t pos = 0;
    int i;

    for (i = 0; i < length; ++i) {
        uint8_t word = pos / 32;
        uint32_t val = in[pos / 32];

        val >>= pos % 32;
        val &= (1 << bits) - 1;

        if ((pos + bits) % 32 < bits) {
            uint32_t val2 = in[word + 1];

            val2 &= (1 << ((pos + bits) % 32)) - 1;
            val2 <<= bits - ((pos + bits) % 32);
            val |= val2;
        }

        val ^= salt % CHARSET_SIZE;
        val %= CHARSET_SIZE;

        out[i] = charset[val];
        ++charsetStats[val];
        pos += bits;
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
