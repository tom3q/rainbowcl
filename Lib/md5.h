#ifndef _MD5_H
#define _MD5_H

#include <stdint.h>

#define MD5_DIGEST_LEN 16

extern void md5(const void *initial_msg, size_t initial_len, uint32_t *digest);

#endif
