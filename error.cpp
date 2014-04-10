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

extern const int MAX_TEXT = 32;

// error class for compiler-usage errors (never prompted by user of
// compiler). Hence, terminate() compiling (influenced by Kerrisk)
#ifdef __GNUC__
__attribute__ ((__noreturn__)) // in case of being called from
#endif                        // non-void function

void
errExit(int pError, const char* format, ...)
{
    va_list arglist;
    char usrMsg[MAX_TEXT+1], errMsg[MAX_TEXT+1], str[MAX_TEXT+1];
    char err[MAX_TEXT];

    va_start(arglist, format);
    vsnprintf(usrMsg, MAX_TEXT, format, arglist);
    va_end(arglist);

    if (pError){
        if ( (errno > 0) && (errno < MAX_ENAME) )
            strcpy(err, ename[errno]);
        else
            strcpy(err, "???");
        snprintf(errMsg, MAX_TEXT, "%s %s", err, strerror(errno));
    }
    else
        strcpy(errMsg, " ");

    // could be too long for str; ignored
    snprintf(str, MAX_TEXT, "ERROR: %s %s\n", usrMsg, errMsg);

    fflush(stdout);
    fputs(str, stderr);
    fflush(stderr);

    exit(EXIT_FAILURE);
}

