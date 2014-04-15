/******************************************************
* error.cpp -   fatal error function                  *
*                                                     *
******************************************************/

#include <cstring>
#include <cstdlib>     // exit(), etc.
#include <cstdio>      // perror()
#include <cstdarg>
#include <errno.h>     // errno()

#include "ename.h"

const int MAX_MSG = 96;

// error class for compiler-usage errors (never prompted by user of
// compiler). Hence, terminate() compiling (influenced by Kerrisk)
#ifdef __GNUC__
__attribute__ ((__noreturn__)) // in case of being called from
#endif                        // non-void function
void
errExit(int pError, const char* format, ...)
{
    va_list arglist;
    char usrMsg[MAX_MSG+1], errMsg[MAX_MSG+1], str[MAX_MSG+1];
    char err[MAX_MSG];

    va_start(arglist, format);
    vsnprintf(usrMsg, MAX_MSG, format, arglist);
    va_end(arglist);

    if (pError){
        if ( (errno > 0) && (errno < MAX_ENAME) )
            strcpy(err, ename[errno]);
        else
            strcpy(err, "???");
        snprintf(errMsg, MAX_MSG, "%s %s", err, strerror(errno));
    }
    else
        strcpy(errMsg, " ");

    // could be too long for str; ignored
    snprintf(str, MAX_MSG, "ERROR: %s %s\n", usrMsg, errMsg);

    fflush(stdout);
    fputs(str, stderr);
    fflush(stderr);

    exit(EXIT_FAILURE);
}

