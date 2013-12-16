#ifndef _RAINBOW_CHAIN_H
#define _RAINBOW_CHAIN_H

#include "utils.h"

// Rainbow Table file format:
// ----------------------------------------------------------------------
// chain length : MD5 hash : password
// ----------------------------------------------------------------------
// uint32_t     : hash_t   : password_t
struct rainbow_chain {
    hash_t hash;
    password_t password;
};

#endif
