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

int no_lex_Errors = 0;
int no_par_Errors = 0;

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

void // TO DO: update when ready
startParse(void)
{
    int is_Clean = 1;
    getNextToken();
    while (*input){
	try{
	    std::cout << "\t\t\tstarting a try round...\n";
	    if ( (1 == is_Clean) )
		match(0, tok_paropen, 1);
	    is_Clean = 1; // until next error
	    pFirst_Node = parseBlock();

	    std::cout << "\n";
	    printSTInfo();
	    pFirst_Node->accept(new MakeIR_Visitor);
	    printIR_List();
	    getNextToken();
	}
	catch(Lexer_Error& m){ // **TO DO: different error class treatment?**
	    m.print();         // (call only Lexer/Semantic)
	    panicModeFwd();
	    is_Clean = 0;
	}
	catch(Primary_Error& m){
	    m.print();
	    panicModeFwd();
	    is_Clean = 0;
	}
	catch(Punct_Error& m){
	    m.print();
	    panicModeFwd();
	    is_Clean = 0;
	}
	catch(VarAccess_Error& m){
	    m.print();
	    panicModeFwd();
	    is_Clean = 0;
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

