#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "utils.h"

// parallel settings
#define OPENMP_MODE 1
#define INTEL_CILK_MODE 0

#if OPENMP_MODE == 1 && INTEL_CILK_MODE == 1
    #error "Cannot compile in both OpenMP and Intel Cilk+ mode"
#endif


#define NUMBER_OF_PASSWORDS     1000
#define LENGTH_OF_CHAIN         1000

const char** uniquePasswords[NUMBER_OF_PASSWORDS];

// Rainbow Table file format:
// ----------------------------------------------------------------------
// password   : MD5 hash   : number of iterations from password to hash
// ----------------------------------------------------------------------
// password_t : checksum_t : unsigned int

// stores one rainbow table row into file
void storeTableRow(const password_t password, checksum_t checksum, unsigned int iterations);

// generates random string with MAX_PASSWD length
void randomString(char* out);

// generates one rainbow table chain, results in ont rainbow table row
void generateRainbowTableChain(const password_t password);

// checks is generated password is unique in rainbow table
// returns 1 if true, 0 otherwise
int isPasswordUnique(const password_t password);

// saves unique password
void saveUniquePassword(const password_t password);

int main(void)
{
    int i;

    // init program
    srand(time(NULL));
    clock_t startTime = clock();

#if OPENMP_MODE == 1
    printf("OpenMP mode selected.\n");
    #pragma omp parallel for
#elif INTEL_CILK_MODE == 1
    printf("Intel Cilk+ mode selected.\n");
#else
    printf("No parallel mode selected.\n");
#endif
    for(i = 0; i < NUMBER_OF_PASSWORDS; ++i)
    {
        password_t randomPassword;
        randomString(randomPassword);
        while(!isPasswordUnique(randomPassword))
            randomString(randomPassword);
        saveUniquePassword(randomPassword);

        generateRainbowTableChain(randomPassword);
    }

    clock_t endTime = clock();
    float workTimeSeconds = (float) (endTime - startTime) / CLOCKS_PER_SEC;
    printf("Work time: %.2f sec\n", workTimeSeconds);

    for(i = 0; i < NUMBER_OF_PASSWORDS; ++i)
        free(uniquePasswords[i]);

    return 0;
}

void storeTableRow(const password_t password, checksum_t checksum, unsigned int iterations)
{
    FILE* file = fopen(TABLE_FILE, "a+");
    if(!file)
    {
        perror("Error opening file");
        exit(1);
    }
    
    fprintf(file, "%s : %08x%08x%08x%08x : %d\n", password, checksum[0], checksum[1], checksum[2], checksum[3], iterations);
    fclose(file);
}

void randomString(char* out)
{
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    size_t length = MAX_PASSWD;
    while(length-- > 0)
    {
        size_t index = (size_t) ((double) rand() / RAND_MAX * (sizeof(charset) - 1));
        *out++ = charset[index];
    }

    *out = '\0';
}

void generateRainbowTableChain(const password_t initialPassword)
{
    checksum_t md5_digest;
    int i;

    password_t chainPassword;
    strcpy(chainPassword, initialPassword, MAX_PASSWD);
    chainPassword[MAX_PASSWD] = '\0';

    for(i = 0; i < LENGTH_OF_CHAIN; ++i)
    {
        checksum(md5_digest, chainPassword);
        reduce(chainPassword, md5_digest);
    }

    storeTableRow(initialPassword, md5_digest, LENGTH_OF_CHAIN);
}

int isPasswordUnique(const password_t password)
{
    int i;
    for(i = 0; i < NUMBER_OF_PASSWORDS; ++i)
    {
        if(uniquePasswords[i] == NULL)
            return 1;

        if(!strcmp(uniquePasswords[i], password))
            return 1;
    }

    return 0;
}

void saveUniquePassword(const password_t password)
{
    int i;
    for(i = 0; i < NUMBER_OF_PASSWORDS; ++i)
    {
        if(uniquePasswords[i])
            continue;

        uniquePasswords[i] = (const char*) malloc(MAX_PASSWD + 1);
        strcpy(uniquePasswords[i], password);
        break;
    }
}