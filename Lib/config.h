#ifndef _CONFIG_H
#define _CONFIG_H

#define MAX_PASSWD              8

// parallel settings
#define OPENMP_MODE             1
#define CILK_MODE               0

#if OPENMP_MODE && CILK_MODE
    #error "Cannot compile in both OpenMP and Cilk+ mode"
#endif

#endif
