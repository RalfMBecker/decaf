/********************************************************************
* driver.cpp - driver for Decaf
*
********************************************************************/

#include <cstdio>
#include <sstream>
#include <cstdio>
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"
#include "parser.h"

extern std::istream* input;

int no_lex_Errors = 0;
int no_par_Errors = 0;

void
initFrontEnd(std::string Str)
{
    makeBinOpTable();
    makeTypePrecTable();
    makeWidthTable(); 
    makeLogArithmTable();
    makeEnvRootTop();

    std::string tmp_Str = ("" == Str)?"std::cin":Str; 
    std::cout << "-----------------------------------------------\n";
    std::cout << "code generated for " << tmp_Str << "\n";
    std::cout << "-----------------------------------------------\n";
}

void // change to a list type ***TO DO***
collectParts() // will collect parts (functions, classes, main,...)
{

}

void // TO DO: update when ready
startParse(void)
{
    top_Env = addEnv(top_Env);
    IdExpr_AST* a = new IdExpr_AST(token(tok_int), token(tok_ID, "a"));
    addIdToEnv(top_Env, a, "stack");
    IdExpr_AST* b = new IdExpr_AST(token(tok_int), token(tok_ID, "b"));
    addIdToEnv(top_Env, b, "stack");

    getNextToken();
    while (*input){
	try{
	    parseExpr();
	}
	catch(Lexer_Error& m){
	    m.print();
	    panicModeFwd();
	}

	catch(Primary_Error& m){
	    std::cout << "Primary_Error\n";
	    m.print();
	    panicModeFwd();
	}
	catch(Punct_Error& m){
	    m.print();
	    panicModeFwd();
	}
    }

    if (no_lex_Errors)
	std::cerr << "found " << no_lex_Errors << " lexical errors\n";

    if (no_par_Errors)
	std::cerr << "found "<< no_par_Errors << " syntactic/semantic errors\n";
}

void // ***TO DO: better a method for each Error type (as we forward
panicModeFwd(void) // in different ways)
{
    int c;

    if ( ('\n' != last_Char) && (EOF != last_Char) ){
	while( ('\n' != (c = getNext())) && (';' != c) )
	    ;
    }

    if (EOF != last_Char)
	getNext();
}

