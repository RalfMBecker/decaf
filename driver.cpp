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

/*
#include <iostream>

#include "parser.h"

void 
HandleDefinition(){
  if ( ParseDefinition() )
    std::cout << "...parsed a function definition.\n";
  else
    getNextToken();
}

void 
HandleExtern(){
  if ( ParseExtern() )
    std::cout << "...parsed an extern statement.\n";
  else
    getNextToken();
}

void 
HandleTopLevelExpression(){
  if ( ParseTopLevelExpr() )
    std::cout << "...parsed a top-level expression.\n";
  else
    getNextToken();
}

void 
MainLoop(){
  for (;;){
    std::cout << "ready> ";
    switch(CurTok){
    case tok_eof:     return;
    case ';':         getNextToken(); break; // ignore at top level
    case tok_def:     HandleDefinition(); break;
    case tok_extern:  HandleExtern(); break;
    default:          HandleTopLevelExpression(); break;
    }
  }
}
*/
