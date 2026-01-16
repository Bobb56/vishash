#ifndef VISHASH_H
#define VISHASH_H

//#define DEBUG

#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define debug_log(verbose, ...)  \
    if (verbose) printf(__VA_ARGS__)

#define DEFAULT_K           125
#define DEFAULT_WIDTH       128
#define DEFAULT_HEIGHT      128
#define DEFAULT_NJOBS       4
#define DEFAULT_VERBOSE     false


#endif