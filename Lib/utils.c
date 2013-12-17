#include "utils.h"

// user-defined constant
const char charset[CHARSET_SIZE] = "0123456789,."
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// generated from charset above using ReduceGenerator
const char reductionMap[REDUCTION_TABLE_SIZE] =
                                    "lWLJHIQP0R0nZE0B2y5RYdU6OdSvfkIs"
                                    "1Xf0AZLmpkxKoHsc97DcZG6vgh4nb5uK"
                                    "RlQquB8GbOwBEpBggTvzUS6OTl9cAw3."
                                    "oZhyAg3M1a25V84xGQ1zpf5l4gMGtWSE"
                                    "N,ZiM6cPnp6,svNxLWCq6Qcy32kYWCQm"
                                    "ElvV38NNB8ehWm,2rPyLTrMPqHVxakrb"
                                    "IUNoZpGdru1xVwz.,f19tOHizRATqrzX"
                                    "afhFS1ChYLsK7rqmOF2IJDXjCXj1Sim3"
                                    "ur7TUoMPs87VegTPEpA5BPwdDgtAVHwF"
                                    "SxCKOSzL.Rnv,xikuZIz2jtbbN3lwUoE"
                                    "kh9FV4EIoHvqaJ80bUejt5DDjmo9aKc0"
                                    "Hh,MsTGnFYyS4RemCLDFUn9WENBJqOYc"
                                    "Kafliv7OJeQe5rF7g76Y12MHdAaDuGft"
                                    "8yKp8XV7yPJsL,jI..b2liyk.RXqXWQu"
                                    "zhpj.u9WfNwBoaM53YA0iQnsebYXT,Cc"
                                    "6t.GdjxFdmC4i93RK0Z4DwkJU4dneItJ";

void reduce(password_t out, hash_t const in, size_t length, uint32_t salt)
{
    unsigned int i;

    for (i = 0; i < (length + 2) / 3; ++i) {
        uint32_t word = in[i] + salt;
        unsigned int j;

        for (j = 0; j < 3 && 3 * i + j < length; ++j) {
            uint32_t index = word % REDUCTION_TABLE_SIZE;

            out[3 * i + j] = reductionMap[index];
            word /= REDUCTION_TABLE_SIZE;
        }
    }
}

void printHash(char *out, const hash_t hash)
{
    int i;

    for (i = 0; i < 4; ++i) {
        sprintf(&out[8 * i], "%02x%02x%02x%02x",
                hash[i] & 0xff,
                (hash[i] >> 8) & 0xff,
                (hash[i] >> 16) & 0xff,
                (hash[i] >> 24) & 0xff);
    }
}
