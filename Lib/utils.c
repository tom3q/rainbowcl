#include "utils.h"

const char charset[CHARSET_SIZE] = "0123456789,."
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

uint32_t charsetStats[CHARSET_SIZE] = { 0, };

void reduce(password_t out, hash_t const in, size_t length, uint32_t salt)
{
    unsigned int i;

    for (i = 0; i < (length + 2) / 3; ++i) {
        uint32_t word = in[i] + salt;
        unsigned int j;

        for (j = 0; j < 3 && 3 * i + j < length; ++j) {
            uint32_t index = word % (8 * CHARSET_SIZE);

            ++charsetStats[index % CHARSET_SIZE];
            out[3 * i + j] = charset[index % CHARSET_SIZE];
            word /= (8 * CHARSET_SIZE);
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
