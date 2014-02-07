#ifndef _CONFIG_H
#define _CONFIG_H

#define MAX_PASSWD              16

// parallel settings
#define OPENMP_MODE             0
#define CILK_MODE               0
#define USE_PIPELINING          0
#define OPENCL_MODE             1

#if (OPENMP_MODE + CILK_MODE + OPENCL_MODE) > 1
    #error "Only one parallel mode can be enabled at the same time"
#endif

#if CILK_MODE
    #define PIPELINING              (USE_PIPELINING)
#else
    #if USE_PIPELINING
        #error Pipelining is supported only in CILK_MODE!
    #endif
    #define PIPELINING              0
#endif

#define USE_VECTORS

#endif
