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

#if OPENMP_MODE
#warning Compiling in OpenMP mode...
#endif

#if CILK_MODE
#warning Compiling in Cilk+ mode...
#include <cilk/cilk.h>
#endif

#if OPENCL_MODE
#warning Compiling in OpenCL mode...
#include <CL/cl.h>
#endif

struct args {
    uint32_t chainsInBlock;
    uint32_t chainLength;
    uint32_t passwordLength;
    uint32_t numberOfChains;
    uint32_t showDist;
};

static sfmt_t sfmt;

static inline void blockFileName(char *out, struct args *args,
                                 uint32_t blockNumber)
{
    sprintf(out, "rainbow-len%u-%03u.tmp",
            args->passwordLength, blockNumber);
}

static inline void outFileName(char *out, struct args *args)
{
    sprintf(out, "rainbow-len%u.tbl", args->passwordLength);
}

// stores one rainbow table row into file
static void storeTableChains(struct args *args, struct rainbow_chain *chain,
                             uint32_t blockNumber)
{
    char filename[256];

    blockFileName(filename, args, blockNumber);

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

// generates initial password for a block of rainbow chains
static void prepareBlock(struct args *args, struct rainbow_chain *chains)
{
    int i;

    for (i = 0; i < args->chainsInBlock; ++i)
    {
        randomString(chains[i].password, args->passwordLength);
    }
}

#if !OPENCL_MODE
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
#endif

static int chainCompare(const void *a, const void *b)
{
    const struct rainbow_chain *ca = a;
    const struct rainbow_chain *cb = b;

    return memcmp(ca->hash, cb->hash, sizeof(ca->hash));
}

// saves a block of random chains to file
static void saveBlock(struct args *args, struct rainbow_chain *chains,
                      uint32_t blockNumber)
{
    qsort(chains, args->chainsInBlock, sizeof(*chains), chainCompare);
    storeTableChains(args, chains, blockNumber);
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
        } else if (!strcmp(argv[i], "-d")) {
            args->showDist = 1;
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
        blockFileName(filename, args, i);
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

#if OPENCL_MODE
static cl_context opencl_context;
static cl_command_queue opencl_queue[2];
static cl_program opencl_program;
static cl_kernel opencl_kernel;
static cl_mem opencl_in_mem[2];
static cl_mem opencl_out_mem[2];
static uint8_t *tmp_buf;
static int passwordSize;
static int hashSize;

// generates initial password for a block of rainbow chains
static void prepareBlockCl(struct args *args, struct rainbow_chain *chains,
                           int index)
{
    uint8_t *tmp = tmp_buf;
    cl_int error;
    int i;

    prepareBlock(args, chains);

    memset(tmp, 0, passwordSize * args->chainsInBlock);

    for (i = 0; i < args->chainsInBlock; ++i) {
        memcpy(tmp, chains[i].password, args->passwordLength);
        tmp += passwordSize;
    }

    error = clEnqueueWriteBuffer(opencl_queue[index], opencl_in_mem[index],
                                 CL_TRUE, 0, passwordSize * args->chainsInBlock,
                                 tmp_buf, 0, NULL, NULL);
    assert(error == CL_SUCCESS);
}

// generates all rainbow chains in a block of rainbow chains
static void processBlockCl(struct args *args, struct rainbow_chain *chains,
                           int index)
{
#ifdef USE_VECTORS
    const size_t globalWorkSize[] = { args->chainsInBlock / 4, 0, 0 };
#else
    const size_t globalWorkSize[] = { args->chainsInBlock, 0, 0 };
#endif
    cl_int error;

    clSetKernelArg(opencl_kernel, 0, sizeof(cl_mem), &opencl_out_mem[index]);
    clSetKernelArg(opencl_kernel, 1, sizeof(cl_mem), &opencl_in_mem[index]);
    clSetKernelArg(opencl_kernel, 2, sizeof(*args), args);

    error = clEnqueueNDRangeKernel(opencl_queue[index], opencl_kernel, 1, NULL,
                                   globalWorkSize, NULL, 0, NULL, NULL);
    assert(error == CL_SUCCESS);

    clFlush(opencl_queue[index]);
}

// makes sure that block processing finished
static void finishBlockCl(int index)
{
    clFinish(opencl_queue[index]);
}

// saves a block of random chains to file
static void saveBlockCl(struct args *args, struct rainbow_chain *chains,
                        uint32_t blockNumber, int index)
{
    uint8_t *tmp = tmp_buf;
    cl_int error;
    int i;

    error = clEnqueueReadBuffer(opencl_queue[index], opencl_out_mem[index],
                                CL_TRUE, 0, hashSize * args->chainsInBlock,
                                tmp_buf, 0, NULL, NULL);
    assert(error == CL_SUCCESS);

    for (i = 0; i < args->chainsInBlock; ++i) {
        memcpy(chains[i].hash, tmp, sizeof(chains->hash));
        tmp += hashSize;
    }

    saveBlock(args, chains, blockNumber);
}

static size_t loadKernel(char **retSrcBuf)
{
    char buf[4096];
    FILE *source;
    size_t ret;
    size_t size = 0;
    size_t srcBufSize = 4096;
    char *srcBuf;

    source = fopen("rainbow.cl", "rb");
    if (!source) {
        fprintf(stderr, "failed to open kernel source code (rainbow.cl)\n");
        return 0;
    }

    srcBuf = malloc(srcBufSize);
    assert(srcBuf);

    while (!feof(source)) {
        ret = fread(buf, 1, sizeof(buf), source);
        if (!ret)
            break;

        if (size + ret > srcBufSize) {
            srcBufSize *= 2;
            srcBuf = realloc(srcBuf, srcBufSize);
            assert(srcBuf);
        }

        memcpy(srcBuf + size, buf, ret);
        size += ret;
    }

    fclose(source);

    *retSrcBuf = srcBuf;
    return size;
}

static int initOpenCL(struct args *args)
{
    cl_uint platformIdCount = 0;
    cl_platform_id *platformIds;
    cl_platform_id platformId;
    cl_uint deviceIdCount = 0;
    cl_device_id *deviceIds;
    cl_device_id deviceId;
    cl_context_properties contextProperties[] = {
        CL_CONTEXT_PLATFORM,
        0, 0, 0
    };
    cl_int error = 0;
    char *srcBuf;
    size_t srcSize;
    int i;

    clGetPlatformIDs(0, NULL, &platformIdCount);
    if (platformIdCount == 0) {
        fprintf(stderr, "no OpenCL platforms found\n");
        return -1;
    }

    platformIds = calloc(platformIdCount, sizeof(*platformIds));
    assert(platformIds);

    clGetPlatformIDs(platformIdCount, platformIds, NULL);
    platformId = platformIds[0];
    free(platformIds);

    clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, 0, NULL, &deviceIdCount);
    if (deviceIdCount == 0) {
        fprintf(stderr, "no OpenCL device found\n");
        return -1;
    }

    deviceIds = calloc(deviceIdCount, sizeof(*deviceIds));
    assert(deviceIds);

    clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, deviceIdCount,
                   deviceIds, NULL);
    deviceId = deviceIds[0];

    contextProperties[1] = (intptr_t)platformId;

    opencl_context = clCreateContext(contextProperties, deviceIdCount,
                                     deviceIds, NULL, NULL, &error);

    if (error != CL_SUCCESS) {
        fprintf(stderr, "OpenCL error %d at %s:%d\n",
                error, __FILE__, __LINE__);
        goto err_free_device_ids;
    }

    for (i = 0; i < 2; ++i) {
        opencl_queue[i] = clCreateCommandQueue(opencl_context, deviceId,
                                               0, &error);
        if (error != CL_SUCCESS) {
            fprintf(stderr, "OpenCL error %d at %s:%d\n",
                    error, __FILE__, __LINE__);
            goto err_destroy_queues;
        }
    }

    srcSize = loadKernel(&srcBuf);
    if (!srcSize) {
        fprintf(stderr, "failed to load OpenCL kernel\n");
        goto err_destroy_queues;
    }

    opencl_program = clCreateProgramWithSource(opencl_context,
                                               1, (const char **)&srcBuf,
                                               &srcSize, &error);
    if (error != CL_SUCCESS) {
        fprintf(stderr, "OpenCL error %d at %s:%d\n",
                error, __FILE__, __LINE__);
        goto err_free_source;
    }

    error = clBuildProgram(opencl_program, deviceIdCount, deviceIds,
                           NULL, NULL, NULL);
    if (error != CL_SUCCESS) {
        // Determine the size of the log
        size_t log_size;

        clGetProgramBuildInfo(opencl_program, deviceId, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(opencl_program, deviceId, CL_PROGRAM_BUILD_LOG,
                              log_size, log, NULL);

        // Print the log
        fprintf(stderr, "OpenCL program compilation error:\n");
        fprintf(stderr, "%s\n", log);

        goto err_free_program;
    }

    opencl_kernel = clCreateKernel(opencl_program, "rainbow", &error);
    if (error != CL_SUCCESS) {
        fprintf(stderr, "OpenCL error %d at %s:%d\n",
                error, __FILE__, __LINE__);
        goto err_free_program;
    }

    /* Keep array elements aligned */
    passwordSize = (args->passwordLength + 15) & ~15;
    hashSize = (sizeof(hash_t) + 15) & ~15;

    for (i = 0; i < 2; ++i) {
        opencl_in_mem[i] = clCreateBuffer(opencl_context,
                                          CL_MEM_READ_ONLY,
                                          passwordSize * args->chainsInBlock,
                                          NULL, &error);
        if (error != CL_SUCCESS) {
            fprintf(stderr, "OpenCL error %d at %s:%d\n",
                    error, __FILE__, __LINE__);
            goto err_free_buffers;
        }

        opencl_out_mem[i] = clCreateBuffer(opencl_context,
                                          CL_MEM_WRITE_ONLY,
                                          hashSize * args->chainsInBlock,
                                          NULL, &error);
        if (error != CL_SUCCESS) {
            fprintf(stderr, "OpenCL error %d at %s:%d\n",
                    error, __FILE__, __LINE__);
            goto err_free_buffers;
        }
    }

    tmp_buf = calloc(args->chainsInBlock, passwordSize > hashSize ?
                                                    passwordSize : hashSize);
    assert(tmp_buf);

    free(srcBuf);
    free(deviceIds);

    return 0;

err_free_buffers:
    for (i = 0; i < 2; ++i) {
        if (opencl_in_mem[i])
            clReleaseMemObject(opencl_in_mem[i]);
        if (opencl_out_mem[i])
            clReleaseMemObject(opencl_out_mem[i]);
    }

    clReleaseKernel(opencl_kernel);

err_free_program:
    clReleaseProgram(opencl_program);

err_free_source:
    free(srcBuf);

err_destroy_queues:
    for (i = 0; i < 2; ++i)
        if (opencl_queue[i])
            clReleaseCommandQueue(opencl_queue[i]);

err_free_device_ids:
    free(deviceIds);

    clReleaseContext(opencl_context);

    return -1;
}

static void releaseOpenCL(void)
{
    int i;

    free(tmp_buf);

    for (i = 0; i < 2; ++i) {
        clReleaseMemObject(opencl_in_mem[i]);
        clReleaseMemObject(opencl_out_mem[i]);
    }

    clReleaseKernel(opencl_kernel);
    clReleaseProgram(opencl_program);

    for (i = 0; i < 2; ++i)
        clReleaseCommandQueue(opencl_queue[i]);
    clReleaseContext(opencl_context);
}
#endif

// entry point of the program
int main(int argc, char **argv)
{
    uint64_t startTime, totalTime;
    uint64_t numberOfPasswords;
    uint32_t numberOfBlocks;
    float workTimeSeconds;
    struct args args;
    uint32_t i;
    uint32_t min, max;
    int current = 0;

#if PIPELINING
    struct rainbow_chain *chains[3];

#else
    struct rainbow_chain *chains[1];

#endif

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
    args.numberOfChains = numberOfBlocks * args.chainsInBlock;

    printf("Generating rainbow table for %u-character passwords\n",
                                              args.passwordLength);
    printf("Total number of passwords: %lu\n", numberOfPasswords);
    printf("Length of rainbow chain:   %u\n", args.chainLength);
    printf("Number of rainbow chains:  %u\n", args.numberOfChains);
    printf("Rainbow chain block size:  %u\n", args.chainsInBlock);
    printf("Number of chain blocks:    %u\n", numberOfBlocks);
    printf("Estimated password coverage: %f%%\n", 100.0f *
                args.numberOfChains * args.chainLength / numberOfPasswords);

    chains[0] = malloc(args.chainsInBlock * sizeof(**chains));
    assert(chains[0]);

#if PIPELINING
    chains[1] = malloc(args.chainsInBlock * sizeof(**chains));
    assert(chains[1]);
    chains[2] = malloc(args.chainsInBlock * sizeof(**chains));
    assert(chains[2]);

#endif

#if PIPELINING
    prepareBlock(&args, chains[0]);

#endif

#if OPENCL_MODE
    assert(initOpenCL(&args) == 0);
    prepareBlockCl(&args, chains[0], 0);

#endif

    current = 0;
    for (i = 0; i < numberOfBlocks; ++i) {

#if PIPELINING
        uint32_t next = (current + 1) % 3;

#endif

        printf("Processing block %d of %d...\n", i + 1, numberOfBlocks);

#if PIPELINING && CILK_MODE
        if (i != numberOfBlocks - 1)
            cilk_spawn prepareBlock(&args, chains[next]);
        processBlock(&args, chains[current]);
        cilk_sync;
        cilk_spawn saveBlock(&args, chains[current], i);

#elif OPENCL_MODE
        finishBlockCl((i - 1) % 2);
        processBlockCl(&args, chains[0], i % 2);
        if (i != 0)
            saveBlockCl(&args, chains[0], i - 1, (i - 1) % 2);
        prepareBlockCl(&args, chains[0], (i + 1) % 2);

#else
        prepareBlock(&args, chains[current]);
        processBlock(&args, chains[current]);
        saveBlock(&args, chains[current], i);

#endif

#if PIPELINING
        current = next;

#endif
    }

#if CILK_MODE
    cilk_sync;

#endif

#if OPENCL_MODE
    finishBlockCl((i - 1) % 2);
    saveBlockCl(&args, chains[0], i - 1, (i - 1) % 2);

#endif

    free(chains[0]);

#if PIPELINING
    free(chains[1]);
    free(chains[2]);

#endif

    sortTables(&args, numberOfBlocks);

    totalTime = measureTime(startTime);
    workTimeSeconds = (float)totalTime / USEC_PER_SEC;
    printf("Work time: %.2f sec\n", workTimeSeconds);

    if (!args.showDist)
        return 0;

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
