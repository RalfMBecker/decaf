/******************************************************
* error.cpp -   fatal error function                  *
*                                                     *
******************************************************/

#include <sstream>
#include <iostream>
#include <cstring>

#include "ename.h"
#include "error.h"

// used from lexer.h
extern int line_No;
extern int col_No;
extern int last_Char;
int getNext(void);

int no_lex_Errors = 0;
int no_par_Errors = 0;

const int MAX_MSG = 96;

// Returns number of scope level adjustments, if any (used in 
// helper adjScopeLevel(int n) in parser.cpp)
// Returning just an int minimizes dependencies of error.cpp.
int
panicModeFwd(void)
{
    int c;
    int adj(0);

// **TO DO: the following line might be necessary for some cases
//     if ( ('\n' != last_Char) && (EOF != last_Char) ){
    if ( (EOF != last_Char) ){
	while( (EOF != (c = getNext())) && (';' != c) ){
	    if ( ('{' == c) )
		adj++;
	    else if ( ('}' == c) )
		adj--;
	}

	if (EOF != last_Char)
	    getNext();

	return adj;
    }
}

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

// is_Error: 0 - warning, 1 - error
void
errorBase(int is_Error)
{
    if (is_Error)
	std::cerr << "Error ";
    else
	std::cerr << "Warning ";
    std::cerr << "near " << line_No << ":" << col_No << ": ";
}

/***************************************
*  Lexer-level error functions
***************************************/

// Type: 0 - regular; 1 - toolong/strtonum
void
lexerError(int Type, std::string const& Name, std::string const& Second)
{
    errorBase(1);

    no_lex_Errors++;
    std::ostringstream tmp_Str;
    tmp_Str << "token (" << Name << ")";
    if ( ("" == Second) ){
	if ( (0 == Type) )
	    tmp_Str << "\n";
	else
	    tmp_Str << " ";
    }
    else
	tmp_Str << " - " << Second << "\n";

    std::cerr << tmp_Str.str();
}

// Enforcing max lengths of identifiers, etc. Sample usage:
//            name=<identifier>, type_Str="MAX_ID", type=MAX_ID
void
tooLongError(std::string const& Name, std::string const& type_Str, int Type)
{    
    lexerError(1, Name, "");
    std::cerr << "longer than " << type_Str << " (" << Type << ")\n";
}

// strtod has arcane error repoting; c./ man page
void
strToNumError(std::string const& Name, std::string const& Type, char C)
{
    lexerError(1, Name, "");
    if ( (0 != errno) ){ // overflow/underflow
	std::cerr << Type << " - ";
	if ( ("float" == Type) )
	    perror("strtod");
	else if ( ("integer" == Type) )
	    perror("strtol");
	else
	    errExit(0, "invalid use of function StrToNum_Error\n");
    }
    else if ( ('\0' != C) )
	std::cerr << " - offending character: " << C << "\n";
}

/***************************************
*  Parser-level error functions
***************************************/

// Type: 0 - not defined; 1 - attempt to redefine
void
varAccessError(std::string Name, int Type)
{
    errorBase(1);
    no_par_Errors++;

    std::ostringstream tmp_Stream;
    tmp_Stream << "attempt to ";
    if ( (0 == Type) )
	tmp_Stream << "access undefined ";
    else if ( (1 == Type) )
	tmp_Stream << "redefine already defined ";
    else
	errExit(0, "illegal use of varAccessError()");
    tmp_Stream << "variable (" << Name << ")\n";

    std::cerr << tmp_Stream.str();
}

void
parseError(std::string tok_Str, std::string com_Str) 
{
    errorBase(1);
    no_par_Errors++;

    std::ostringstream tmp_Str;
    tmp_Str << "Syntax error (" << tok_Str << ") - " << com_Str << "\n";
 
    std::cerr << tmp_Str.str();
}

void
parseWarning(std::string tok_Str, std::string com_Str) 
{
    errorBase(0);
    no_par_Errors++;

    std::ostringstream tmp_Str;
    tmp_Str << "In (" << tok_Str << ") - " << com_Str << "\n";
 
    std::cerr << tmp_Str.str();
}

// What - 0: missing; 1: spare
void
punctError(char C, int What)
{
    if ( (0!= What) && (1!= What) )
	errExit(0, "invalid arg of Punct_Error (%d)\n", What); 

    errorBase(1);
    no_par_Errors++;

    std::ostringstream tmp_Str;
    tmp_Str << "Syntax error - ";
    if ( (0 == What) ) tmp_Str << "missing ";
    else tmp_Str << "stray ";
    tmp_Str << "token (" << C << ")\n";

    std::cerr << tmp_Str.str();
}
