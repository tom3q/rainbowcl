#include <assert.h>
#include <stdio.h>

#include "rainbow_chain.h"

int main(int argc, char **argv)
{
    struct rainbow_chain chain;
    FILE *f;
    char hash[33];

    f = fopen(argv[1], "rb");
    assert(f);

    while (fread(&chain, sizeof(chain), 1, f) == 1) {
        printHash(hash, chain.hash);
        printf("%s : %s\n", chain.password, hash);
    }

    fclose(f);

    return 0;
}
