#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"

int main(int argc, char* argv[])
{
    const char* initial_msg = "wypierdalaj dziadu z PORR...";
    uint8_t* md5_digest = (uint8_t*) malloc(MD5_DIGEST_LEN);

    md5(initial_msg, strlen(initial_msg), md5_digest);

    printf("MD5(%s) = %s", initial_msg, (char*) md5_digest);

    free(md5_digest);
    return 0;
}