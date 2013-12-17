#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "rainbow_chain.h"
#include "time.h"
#include "utils.h"

#define SFMT_MEXP 19937
#include "SFMT.h"

int main(int argc, char **argv)
{
    unsigned int sourceSize = REDUCTION_TABLE_SIZE;
    char *charsetSrc;
    char *output;
    sfmt_t sfmt;
    unsigned int i;

    sfmt_init_gen_rand(&sfmt, time(NULL));

    charsetSrc = malloc(REDUCTION_TABLE_SIZE * sizeof(char));
    assert(charsetSrc);

    output = malloc(REDUCTION_TABLE_SIZE * sizeof(char));
    assert(output);

    assert(REDUCTION_TABLE_SIZE % CHARSET_SIZE == 0);

    for (i = 0; i < REDUCTION_TABLE_SIZE / CHARSET_SIZE; ++i)
        memcpy(charsetSrc + i * CHARSET_SIZE, charset,
                CHARSET_SIZE * sizeof(char));

    for (i = 0; i < REDUCTION_TABLE_SIZE; ++i) {
        uint32_t random;
        uint32_t index;

        random = sfmt_genrand_uint32(&sfmt);
        index = (uint32_t)((double)sourceSize * random / UINT_MAX);
        if (index == sourceSize)
            index = sourceSize - 1;

        output[i] = charsetSrc[index];
        charsetSrc[index] = charsetSrc[--sourceSize];
    }

    printf("const char reductionMap[REDUCTION_TABLE_SIZE] =");
    for (i = 0; i < REDUCTION_TABLE_SIZE / 32; ++i)
        printf("\n                                    \"%.32s\"",
                output + 32 * i);
    printf(";\n");

    free(output);
    free(charsetSrc);
    return 0;
}
