#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "utils.h"

// parallel settings
#define OPENMP_MODE 1
#define CILK_MODE 0

#if OPENMP_MODE && CILK_MODE
    #error "Cannot compile in both OpenMP and Cilk+ mode"
#endif

#define NUMBER_OF_PASSWORDS     1000000000
#define CHAINS_IN_BLOCK         1000
#define LENGTH_OF_CHAIN         10000
#define NUMBER_OF_CHAINS        (NUMBER_OF_PASSWORDS / LENGTH_OF_CHAIN)
#define NUMBER_OF_BLOCKS        (NUMBER_OF_CHAINS / CHAINS_IN_BLOCK)

#if NUMBER_OF_BLOCKS == 0
#undef NUMBER_OF_BLOCKS
#define NUMBER_OF_BLOCKS        1
#endif

// Rainbow Table file format:
// ----------------------------------------------------------------------
// chain length : MD5 hash : password
// ----------------------------------------------------------------------
// uint32_t     : hash_t   : password_t
struct rainbow_chain {
    uint32_t length;
    hash_t hash;
    password_t password;
};

// stores one rainbow table row into file
static void storeTableChain(struct rainbow_chain *chain)
{
    FILE* file = fopen(TABLE_FILE, "ab");
    if(!file)
    {
        perror("Error opening file");
        exit(1);
    }

    fwrite(chain, sizeof(*chain), 1, file);
    fclose(file);
}

// generates random string with up to MAX_PASSWD length
static void randomString(char* out)
{
    static const char charset[] = "0123456789"
                                  "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    size_t length = (size_t) ((double) rand() / RAND_MAX * MAX_PASSWD);
    while(length-- > 0)
    {
        size_t index = (size_t) ((double) rand() /
                                            RAND_MAX * (sizeof(charset) - 1));
        *out++ = charset[index];
    }

    *out = '\0';
}

// generates one rainbow table chain, results in ont rainbow table row
static void generateRainbowTableChain(struct rainbow_chain *chain)
{
    password_t chainPassword;
    hash_t passwordHash;
    int i;

    strcpy(chainPassword, chain->password);

    for(i = 0; i < LENGTH_OF_CHAIN; ++i)
    {
        hash(passwordHash, chainPassword);
        reduce(chainPassword, passwordHash);
    }

    memcpy(chain->hash, passwordHash, sizeof(passwordHash));
    chain->length = i;
}

// generates initial password for a block of rainbow chains
static void prepareBlock(struct rainbow_chain *chains)
{
    int i;

    for (i = 0; i < CHAINS_IN_BLOCK; ++i)
    {
        randomString(chains[i].password);
    }
}

// generates all rainbow chains in a block of rainbow chains
static void processBlock(struct rainbow_chain *chains)
{
    int i;

#if OPENMP_MODE
    #pragma omp parallel for
    for(i = 0; i < CHAINS_IN_BLOCK; ++i)
#elif CILK_MODE
    cilk_for (i = 0; i < CHAINS_IN_BLOCK; ++i)
#else
    for(i = 0; i < CHAINS_IN_BLOCK; ++i)
#endif
    {
        generateRainbowTableChain(&chains[i]);
    }
}

// saves a block of random chains to file
static void saveBlock(struct rainbow_chain *chains)
{
    int i;

    for (i = 0; i < CHAINS_IN_BLOCK; ++i)
    {
        storeTableChain(&chains[i]);
    }
}

// entry point of the program
int main(int argc, char **argv)
{
    uint64_t startTime, totalTime;
    struct rainbow_chain *chains;
    float workTimeSeconds;
    int i;

#if OPENMP_MODE
    printf("OpenMP mode selected.\n");
#elif CILK_MODE
    printf("Intel Cilk+ mode selected.\n");
#else
    printf("No parallel mode selected.\n");
#endif

    FILE* file = fopen(TABLE_FILE, "wb");
    if(!file)
    {
        perror("Error opening file");
        return -1;
    }
    fclose(file);

    // init program
    startTime = measureTime(0);

    chains = malloc(CHAINS_IN_BLOCK * sizeof(*chains));
    assert(chains);

    srand(time(NULL));

    for (i = 0; i < NUMBER_OF_BLOCKS; ++i) {
        printf("Processing block %d of %d...\n", i, NUMBER_OF_BLOCKS);
        prepareBlock(chains);
        processBlock(chains);
        saveBlock(chains);
    }

    totalTime = measureTime(startTime);
    workTimeSeconds = (float)totalTime / USEC_PER_SEC;
    printf("Work time: %.2f sec\n", workTimeSeconds);

    free(chains);

    return 0;
}
