#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "md5.h"
#include "rainbow_chain.h"
#include "utils.h"

#define SFMT_MEXP 19937
#include "SFMT.h"

#define DIV_ROUND_UP(dividend, divisor) \
    (((dividend) + (divisor) - 1) / (divisor))

struct args {
    uint32_t chainsInBlock;
    uint32_t chainLength;
    uint32_t passwordLength;
    uint32_t numberOfChains;
    uint32_t blockNumber;
};

static sfmt_t sfmt;

static inline void blockFileName(char *out, struct args *args)
{
    sprintf(out, "rainbow-len%u-%03u.tmp",
            args->passwordLength, args->blockNumber);
}

static inline void outFileName(char *out, struct args *args)
{
    sprintf(out, "rainbow-len%u.tbl", args->passwordLength);
}

// stores one rainbow table row into file
static void storeTableChains(struct args *args,
                            struct rainbow_chain *chain)
{
    char filename[256];

    blockFileName(filename, args);

    FILE* file = fopen(filename, "wb");
    if(!file)
    {
        perror("Error opening file");
        exit(1);
    }

    fwrite(chain, sizeof(*chain), args->chainsInBlock, file);
    fclose(file);
}

static uint32_t charsetStats[CHARSET_SIZE];

// generates random string with up to MAX_PASSWD length
static void randomString(char* out, size_t length)
{
    while(length-- > 0)
    {
        uint32_t random = sfmt_genrand_uint32(&sfmt);
        size_t index = (size_t) ((double)CHARSET_SIZE * random / UINT_MAX);

        if (index >= CHARSET_SIZE)
            index = CHARSET_SIZE - 1;

        *out++ = charset[index];
        ++charsetStats[index];
    }

    *out = '\0';
}

// generates one rainbow table chain, results in ont rainbow table row
static void generateRainbowTableChain(struct args *args,
                                      struct rainbow_chain *chain)
{
    password_t chainPassword;
    hash_t passwordHash;
    int i;

    strcpy(chainPassword, chain->password);

    for(i = 0; i < args->chainLength; ++i)
    {
        hash(passwordHash, chainPassword);
        reduce(chainPassword, passwordHash, args->passwordLength, i);
    }
    hash(passwordHash, chainPassword);

    memcpy(chain->hash, passwordHash, sizeof(passwordHash));
}

// generates initial password for a block of rainbow chains
static void prepareBlock(struct args *args, struct rainbow_chain *chains)
{
    int i;

    for (i = 0; i < args->chainsInBlock; ++i)
    {
        randomString(chains[i].password, args->passwordLength);
    }
}

// generates all rainbow chains in a block of rainbow chains
static void processBlock(struct args *args, struct rainbow_chain *chains)
{
    int i;

#if OPENMP_MODE
    #pragma omp parallel for
    for(i = 0; i < args->chainsInBlock; ++i)
#elif CILK_MODE
    cilk_for (i = 0; i < args->chainsInBlock; ++i)
#else
    for(i = 0; i < args->chainsInBlock; ++i)
#endif
    {
        generateRainbowTableChain(args, &chains[i]);
    }
}

static int chainCompare(const void *a, const void *b)
{
    const struct rainbow_chain *ca = a;
    const struct rainbow_chain *cb = b;

    return memcmp(ca->hash, cb->hash, sizeof(ca->hash));
}

// saves a block of random chains to file
static void saveBlock(struct args *args, struct rainbow_chain *chains)
{
    qsort(chains, args->chainsInBlock, sizeof(*chains), chainCompare);
    storeTableChains(args, chains);
}

static void parseArgs(struct args *args, int argc, char **argv)
{
    unsigned set = 0;
    int i;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-c")) {
            if (i == argc - 1)
                goto show_usage;
            args->chainLength = atoi(argv[i + 1]);
            ++i;
            set |= (1 << 0);
            continue;
        } else if (!strcmp(argv[i], "-b")) {
            if (i == argc - 1)
                goto show_usage;
            args->chainsInBlock = atoi(argv[i + 1]);
            ++i;
            set |= (1 << 1);
            continue;
        } else if (!strcmp(argv[i], "-l")) {
            if (i == argc - 1)
                goto show_usage;
            args->passwordLength = atoi(argv[i + 1]);
            ++i;
            set |= (1 << 2);
            continue;
        } else if (!strcmp(argv[i], "-n")) {
            if (i == argc - 1)
                goto show_usage;
            args->numberOfChains = atoi(argv[i + 1]);
            ++i;
            set |= (1 << 3);
            continue;
        }
    }

    if (set == 0xf)
        return;

show_usage:
    fprintf(stderr,
            "%s -l password_length -n number_of_chains "
            "-c chain_length -b chains_in_block\n",
            argv[0]);
    exit(1);
}

static void sortTables(struct args *args, uint32_t numberOfBlocks)
{
    struct rainbow_chain *chains;
    char filename[256];
    FILE **blocks;
    FILE *out;
    size_t ret;
    int i;

    chains = malloc(numberOfBlocks * sizeof(*chains));
    assert(chains);

    blocks = malloc(numberOfBlocks * sizeof(*blocks));
    assert(blocks);

    for (i = 0; i < numberOfBlocks; ++i) {
        args->blockNumber = i;
        blockFileName(filename, args);
        blocks[i] = fopen(filename, "rb");
        assert(blocks[i]);

        ret = fread(&chains[i], sizeof(*chains), 1, blocks[i]);
        assert(ret == 1);
    }

    outFileName(filename, args);
    out = fopen(filename, "wb");
    assert(out);

    do {
        int min = 0;

        for (i = 0; i < numberOfBlocks; ++i) {
            if (memcmp(chains[i].hash, chains[min].hash, sizeof(hash_t)) < 0)
                min = i;
        }

        ret = fwrite(&chains[min], sizeof(*chains), 1, out);
        assert(ret == 1);

        ret = fread(&chains[min], sizeof(*chains), 1, blocks[min]);
        if (ret != 1) {
            fclose(blocks[min]);
            --numberOfBlocks;
            blocks[min] = blocks[numberOfBlocks];
            chains[min] = chains[numberOfBlocks];
        }
    } while (numberOfBlocks > 1);

    do {
        ret = fwrite(&chains[0], sizeof(*chains), 1, out);
        assert(ret == 1);
    } while (fread(&chains[0], sizeof(*chains), 1, blocks[0]) == 1);

    fclose(blocks[0]);
    fclose(out);
}

// entry point of the program
int main(int argc, char **argv)
{
    uint64_t startTime, totalTime;
    struct rainbow_chain *chains;
    uint64_t numberOfPasswords;
    uint32_t numberOfBlocks;
    float workTimeSeconds;
    struct args args;
    uint32_t i;
    uint32_t min, max;

    memset(&args, 0, sizeof(args));
    parseArgs(&args, argc, argv);

    assert(args.chainsInBlock);
    assert(args.chainLength);
    assert(args.passwordLength);
    assert(args.numberOfChains);
    assert(args.passwordLength <= MAX_PASSWD);

#if OPENMP_MODE
    printf("OpenMP mode selected.\n");
#elif CILK_MODE
    printf("Intel Cilk+ mode selected.\n");
#else
    printf("No parallel mode selected.\n");
#endif

    // init program
    startTime = measureTime(0);

    sfmt_init_gen_rand(&sfmt, time(NULL));

    numberOfPasswords = CHARSET_SIZE;
    for (i = 1; i < args.passwordLength; ++i) {
        numberOfPasswords *= CHARSET_SIZE;
    }

    numberOfBlocks = DIV_ROUND_UP(args.numberOfChains, args.chainsInBlock);
    if (!numberOfBlocks)
        numberOfBlocks = 1;

    printf("Generating rainbow table for %u-character passwords\n",
                                              args.passwordLength);
    printf("Total number of passwords: %lu\n", numberOfPasswords);
    printf("Length of rainbow chain:   %u\n", args.chainLength);
    printf("Number of rainbow chains:  %u\n", args.numberOfChains);
    printf("Rainbow chain block size:  %u\n", args.chainsInBlock);
    printf("Number of chain blocks:    %u\n", numberOfBlocks);
    printf("Estimated password coverage: %f%%\n", 100.0f *
                args.numberOfChains * args.chainLength / numberOfPasswords);

    chains = malloc(args.chainsInBlock * sizeof(*chains));
    assert(chains);

    for (i = 0; i < numberOfBlocks; ++i) {
        printf("Processing block %d of %d...\n", i + 1, numberOfBlocks);

        if (i == numberOfBlocks - 1
            && args.chainsInBlock > args.numberOfChains)
        {
            args.chainsInBlock = args.numberOfChains;
        }

        args.blockNumber = i;

        prepareBlock(&args, chains);
        processBlock(&args, chains);
        saveBlock(&args, chains);

        args.numberOfChains -= args.chainsInBlock;
    }

    free(chains);

    sortTables(&args, numberOfBlocks);

    totalTime = measureTime(startTime);
    workTimeSeconds = (float)totalTime / USEC_PER_SEC;
    printf("Work time: %.2f sec\n", workTimeSeconds);

    max = 0;
    min = UINT_MAX;
    printf("Random character distribution:\n");
    for (i = 0; i < CHARSET_SIZE; i += 4) {
        unsigned int j;

        for (j = 0; j < 4; ++j) {
            if (charsetStats[i + j] < min)
                min = charsetStats[i + j];
            if (charsetStats[i + j] > max)
                max = charsetStats[i + j];
        }

        printf("%c : %-10u    ", charset[i],     charsetStats[i]);
        printf("%c : %-10u    ", charset[i + 1], charsetStats[i + 1]);
        printf("%c : %-10u    ", charset[i + 2], charsetStats[i + 2]);
        printf("%c : %-10u\n",   charset[i + 3], charsetStats[i + 3]);
    }
    printf("min = %u, max = %u, avg = %u, diff = %u\n",
            min, max, (min + max) / 2, max - min);

    max = 0;
    min = UINT_MAX;
    printf("Reduction character distribution:\n");
    for (i = 0; i < CHARSET_SIZE; i += 4) {
        unsigned int j;

        for (j = 0; j < 4; ++j) {
            if (reductionStats[(uint32_t)charset[i + j]] < min)
                min = reductionStats[(uint32_t)charset[i + j]];
            if (reductionStats[(uint32_t)charset[i + j]] > max)
                max = reductionStats[(uint32_t)charset[i + j]];
        }

        printf("%c : %-10u    ", charset[i],
                reductionStats[(uint32_t)charset[i]]);
        printf("%c : %-10u    ", charset[i + 1],
                reductionStats[(uint32_t)charset[i + 1]]);
        printf("%c : %-10u    ", charset[i + 2],
                reductionStats[(uint32_t)charset[i + 2]]);
        printf("%c : %-10u\n",   charset[i + 3],
                reductionStats[(uint32_t)charset[i + 3]]);
    }
    printf("min = %u, max = %u, avg = %u, diff = %u\n",
            min, max, (min + max) / 2, max - min);

    return 0;
}
