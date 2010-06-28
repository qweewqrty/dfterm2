#include "rng.hpp"
#include <openssl/rand.h>
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

void dfterm::seedRNG()
{
    int rng_counter = 100000;
    /* Seed random number generator a bit */
    while(!RAND_status() && rng_counter > 0)
    {
        rng_counter--;

        #ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>

        RAND_screen();
        DWORD t = GetTickCount();
        RAND_add((void*) &t, sizeof(t), sizeof(t) / 2);
        #else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        RAND_add((void*) &tv, sizeof(tv), sizeof(tv) / 4);
        #endif
    }
}

void dfterm::makeRandomBytes(unsigned char* output, int output_size)
{
    if (!RAND_status()) seedRNG();

    if (!RAND_bytes(output, output_size))
    {
        seedRNG();
        if (!RAND_bytes(output, output_size))
            RAND_pseudo_bytes(output, output_size);
    }
}
