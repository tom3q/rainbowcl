#ifndef _UTILS_H
#define _UTILS_H

#define MAX_PASSWD	8

typedef unsigned int checksum_t[4];
typedef char password_t[MAX_PASSWD + 1];

static inline void checksum(checksum_t *out, const password_t *in)
{

}

static inline void reduce(password_t *out, const checksum_t *in)
{

}

#endif
