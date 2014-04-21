/********************************************************************
* driver.cpp - driver for Decaf
*
********************************************************************/

#include <cstdio> // include max. # of #include files to monitor dependencies
#include <sstream>
#include <cstdio>
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"
#include "parser.h"
//#include "ir.h" // included through visitor.h (if not excluded, 
//                   IR_LIST is twice in same file (erro)
#include "visitor.h"

extern std::istream* input;
extern Node_AST* pFirst_Node; // double declaration (from ast.h) - for clarity

extern int no_lex_Errors;
extern int no_par_Errors;

void
initFrontEnd(std::string Str)
{
    makeBinOpTable();
    makeTypePrecTable();
    makeWidthTable(); 
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

void
startParse(void)
{
    getNextToken();
    pFirst_Node = parseBlock();
    if ( (0 != pFirst_Node) ){
	printSTInfo();
	pFirst_Node->accept(new MakeIR_Visitor);
	printIR_List();
    }
    else
	std::cerr << "---no valid statements found---\n";

    std::string plural;
    if (no_lex_Errors){
	std::cerr << "\nfound " << no_lex_Errors << " lexical error";
	plural = ( (1 < no_lex_Errors) )?"s\n":"\n";
	std::cerr << plural;
    }
    if (no_par_Errors){
	std::cerr << "found "<< no_par_Errors << " syntactic/semantic error";
	plural = ( (1 < no_par_Errors) )?"s\n":"\n";
	std::cerr << plural;
    }
}

