/******************************************************
* compiler.h -   libs for 'input' strean, and         *
*                compiler-wide definitions            *
*                                                     *
******************************************************/

#ifndef COMPILER_H_
#define COMPILER_H_

#include <fstream>     // file streams and operations
#include <iostream>    // cin, cout, cerr, etc.
#include <unistd.h>    // getopt; optarg, optind, opterr, optopt
                       // assumes _POSIX_C_SOURCE >= 2 feature test
                       // (unchecked)
#include <cstdlib>     // exit(); EXIT_FAILURE/EXIT_SUCCESS

// consider change to const ints
#define MAX_ID 31
#define MAX_LIT 32
#define MAX_STR 32

#endif
