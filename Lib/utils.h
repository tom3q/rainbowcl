#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <string.h>
#include "md5.h"

#define TABLE_FILE  "rainbow_table.txt"
#define MAX_PASSWD	8

typedef unsigned int checksum_t[4];
typedef char password_t[MAX_PASSWD + 1];

static void checksum(checksum_t* out, password_t* const in)
{
    md5(*in, strlen(*in), *out);
}

static void reduce(password_t* out, checksum_t* const in)
{

}

static void printDigest(checksum_t* checksum, password_t* const password)
{
    printf("MD5(%s) = %x%x%x%x\n", *password,
            (*checksum)[0], (*checksum)[1], (*checksum)[2], (*checksum)[3]);
}

#endif
