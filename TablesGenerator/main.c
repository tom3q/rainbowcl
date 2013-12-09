#include <stdio.h>

#include "md5.h"
#include "utils.h"

// Rainbow Table file format:
// ----------------------------------------------------------------------
// password   : MD5 hash   : number of iterations from password to hash
// ----------------------------------------------------------------------
// password_t : checksum_t : unsigned int

void storeTableRow(const password_t* password, checksum_t* checksum, unsigned int iterations);

int main(int argc, char* argv[])
{
    checksum_t md5_digest;
    password_t testPassword = "12345678";

    checksum(md5_digest, testPassword);
    printDigest(md5_digest, testPassword);

    storeTableRow(testPassword, md5_digest, 5);

    return 0;
}

void storeTableRow(const password_t* password, checksum_t* checksum, unsigned int iterations)
{
    FILE* file = fopen(TABLE_FILE, "a+");
    if(!file)
    {
        perror("Error opening file");
    }
    
    fprintf(file, "%s : %x%x%x%x : %d\n", password, checksum[0], checksum[1], checksum[2], checksum[3], iterations);
    fclose(file);
}