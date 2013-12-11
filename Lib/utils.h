#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <string.h>
#include "md5.h"

#define TABLE_FILE  "rainbow_table.txt"
#define MAX_PASSWD	8

typedef unsigned int checksum_t[4];
typedef unsigned char password_t[MAX_PASSWD + 1];

static void checksum(checksum_t* out, const password_t* in)
{
    md5(in, strlen(in), out);
}

static void reduce(password_t out, const checksum_t in)
{
    // hacky: this is not a correct reduce function, resulting password may not fit in our character set
    out[0] = (in[0] & 0x00FF0000) >> 16;
    out[1] = in[0] & 0x000000FF;
    out[2] = (in[1] & 0x00FF0000) >> 16;
    out[3] = in[1] & 0x00000FF;
    out[4] = (in[2] & 0x00FF0000) >> 16;
    out[5] = in[2] & 0x000000FF;
    out[6] = (in[3] & 0x00FF0000) >> 16;
    out[7] = in[3] & 0x000000FF;
    out[8] = '\0';
}

static void printDigest(checksum_t* checksum, const password_t* password)
{
    printf("MD5(%s) = %x%x%x%x\n", password, (*checksum)[0], (*checksum)[1], (*checksum)[2], (*checksum)[3]);
}

#endif
