#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"
#include "md5.h"

typedef unsigned int hash_t[4];
typedef char password_t[MAX_PASSWD];

#define REDUCTION_TABLE_SIZE    512
#define CHARSET_SIZE            64
extern const char charset[CHARSET_SIZE + 1];
extern uint32_t reductionStats[256];

void reduce(password_t out, hash_t const in, size_t length, uint32_t salt);

static inline void hash(hash_t out, password_t const in)
{
    md5(in, strlen(in), out);
}

#define USEC_PER_SEC            1000000

// returns current time in microseconds
static inline uint64_t getTime(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
}

// returns a time difference between now and passed argument in microseconds
static inline uint64_t measureTime(uint64_t start)
{
    return getTime() - start;
}

void printHash(char *out, const hash_t hash);
static inline void printfHash(const hash_t hash)
{
    char buf[64];

    printHash(buf, hash);
    printf("%s\n", buf);
}

#endif
