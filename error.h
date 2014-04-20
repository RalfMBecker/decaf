/***********************************************************************
* error.h -   basic error classes                     
*                                                     
* Stroustrup: "(throw-catch) Exception handling is intended for dealing 
*             with non-local problems." (Special Edition, p. 193)
*
* Philosophy: We started out using an Error Class hierarchy, and
*             try-catch blocks in the driver. This does not work (well)
*             for incremental error REPORTS followed by LOCAL error
*             recovery (we keep compiling to catch as many errors in one
*             compile iteration as possible); and local try-catch semantic
*             in the parser would make the code a mess.
* Solution:   use error functions, not classes
* 
* Why not classes and their members, used locally by caller? :
*             Calling member classes of an error class hierarchy from 
*             the parser, say, leads to tricky cross-dependency issues
*             and code that is hard, if not impossible, to compile. 
*
************************************************************************/

#ifndef ERROR_H_
#define ERROR_H_

#include <string>

// for errExit()
#include <cstdlib>     // exit(), strtol(), etc.
#include <cstdio>      // perror()
#include <cstdarg>
#include <errno.h>     // errno()

const int MAX_TEXT = 32;

extern int no_lex_Errors;
extern int no_par_Errors;

int panicModeFwd(void);
void errExit(int pError, const char* format, ...);

void lexerError(int Type, std::string const& Name, std::string const& Second);
void tooLongError(std::string const& Name,std::string const& type_Str,int Type);
void strToNumError(std::string const& Name, std::string const& Type, char C);

void varAccessError(std::string Name, int Type);
void parseError(std::string tok_Str, std::string com_Str);
void parseWarning(std::string tok_Str, std::string com_Str);
void punctError(char C, int What);

#endif
