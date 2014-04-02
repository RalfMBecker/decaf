/********************************************************************
* driver.cpp - driver for Decaf
*
********************************************************************/

#include <cstdio>
#include <sstream>
#include "lexer.h"

extern std::istream* input;

int no_lex_Errors = 0;

void
panicModeFwd(void)
{
    int c;

    if ( ('\n' != last_Char) && (EOF != last_Char) ){
	while( ('\n' != (c = getNext())) && (';' != c) )
	    ;
    }

    if (EOF != last_Char)
	getNext();
}

