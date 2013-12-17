#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "rainbow_chain.h"
#include "utils.h"

static FILE *f;

static int chainCompare(const void *a, const void *b)
{
    const struct rainbow_chain *key = a;
    unsigned long index = (unsigned long)b - 1;
    struct rainbow_chain chain;
    size_t ret;

    fseek(f, index * sizeof(chain), SEEK_SET);
    ret = fread(&chain, sizeof(chain), 1, f);
    assert(ret == 1);

    return memcmp(key->hash, chain.hash, sizeof(key->hash));
}

static void stringToHash(hash_t out, const char *in)
{
    char strByte[3];
    uint32_t word;
    int i;

    strByte[2] = '\0';

    for (i = 0; i < 4; ++i) {
        const char *strWord = &in[i * 8];
        uint32_t byte;
        int j;

        word = 0;

        for (j = 0; j < 4; ++j) {
            memcpy(strByte, strWord + 2 * j, 2);
            sscanf(strByte, "%02x", &byte);
            word >>= 8;
            word |= byte << 24;
        }

        out[i] = word;
    }
}

static int lookupChain(FILE *f, const hash_t initialHash,
                       unsigned long index, struct rainbow_chain *foundChain,
                       uint32_t passwordLength, uint32_t chainLength)
{
    struct rainbow_chain chain;
    size_t read;
    uint32_t i;
    uint32_t offset = 0;
    int backwards = 0;

again:
    if (backwards) {
        if (offset > index)
            return 0;
        i = index - offset;
    } else {
        i = index + offset;
    }

    fseek(f, i * sizeof(chain), SEEK_SET);
    read = fread(&chain, sizeof(chain), 1, f);

    if (read != 1
        || memcmp(chain.hash, foundChain->hash, sizeof(chain.hash)))
    {
        if (backwards) {
            return 0;
        } else {
            backwards = 1;
            offset = 1;
            goto again;
        }
    }

    hash(chain.hash, chain.password);
    for (i = 0; i < chainLength; ++i) {
        if (!memcmp(chain.hash, initialHash, sizeof(chain.hash)))
            goto found;

        reduce(chain.password, chain.hash, passwordLength, i);
        hash(chain.hash, chain.password);
    }

    if (!memcmp(chain.hash, initialHash, sizeof(chain.hash)))
            goto found;

    ++offset;
    goto again;

found:
    memcpy(foundChain->hash, initialHash, sizeof(hash_t));
    strcpy(foundChain->password, chain.password);
    return 1;
}

int main(int argc, char **argv)
{
    struct rainbow_chain chain;
    uint32_t passwordLength;
    uint32_t chainLength;
    unsigned long index;
    hash_t initialHash;
    size_t read;
    uint32_t i;
    long len;

    f = fopen(argv[1], "rb");
    assert(f);

    read = fread(&chain, sizeof(chain), 1, f);
    assert(read == 1);

    passwordLength = strlen(chain.password);
    stringToHash(initialHash, argv[3]);
    chainLength = atoi(argv[2]);

    printf("Looking for hash %s in table %s\n", argv[3], argv[1]);
    printf("Password length is %u\n", passwordLength);

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    len /= sizeof(chain);

    printf("Found %ld rainbow chains in table\n", len);

    for (i = 0; i <= chainLength; ++i) {
        uint32_t j;

        memcpy(chain.hash, initialHash, sizeof(initialHash));

        for (j = 0; j < i; ++j) {
            reduce(chain.password, chain.hash,
                    passwordLength, chainLength - i + j);
            hash(chain.hash, chain.password);
        }

        index = (unsigned long)bsearch(&chain, (void *)1, len, 1, chainCompare);
        if (index && lookupChain(f, initialHash, index - 1,
                                    &chain, passwordLength, chainLength))
            break;
    }

    if (i > chainLength)
        printf("Failed to find password for given hash\n");
    else
        printf("Found password: %s\n", chain.password);

    fclose(f);
    return 0;
}
