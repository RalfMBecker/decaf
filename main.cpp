/********************************************************************
* main.cpp - program entry point
*
* Conventions:
*
* Include order: lexer.h, ast.h, error.h, tables.h, driver.h
*                We favor selective forward declaration instead of
*                including the entire header where possible (e.g.,
*                we only use 'pointer to type')
*
* Object names: class - Foo_Name
*               class var - foo_
*               class member returning foo_ - Foo() or getFoo()
*               var/array/container - foo/foo_Name
*               function - foo/fooName
*               function args - Foo (preferred)/regular var convention
*
********************************************************************/

#include "compiler.h"
#include "lexer.h"
#include "ast.h" // REMOVE - just to keep an eye on it
#include "error.h"
#include "tables.h" // REMOVE - just to keep an eye on it
#include "driver.h"

std::istream* input;

int
main(int argc, char* argv[])
{
    switch(argc){
    case 1:
	input = &std::cin;
	break;
    case 2: 
	input = new std::ifstream(argv[1]);
	if (!input){  // WHY DOES ERROR NOT GET TRIGGERED??????? *********
	    std::cerr << argv[0] << ": can't open file " << argv[1];
	    exit(EXIT_FAILURE);
	}
	break;
    default:
	std::cerr << "Error: # of args\n"; // add basic error fct.  
	break;
    }

    // create ready-state
    if ( (&std::cin == input) )
	std::cout << "ready> ";
    getNext();

    // relocate to driver
    makeBinOpTable();
    makeTypePrecTable();
    makeEnvRootTop();

    // will be addressed when getTok is incorporated into parser
    while (*input){

	try{
	    while ( (EOF != getTok().Tok() ) )
		;
	}

	catch(Error& m){
	    m.print();
	    panicModeFwd();
	}
    }

    if (no_lex_Errors)
	std::cerr << "found " << no_lex_Errors << " lexical errors\n";

    if ( !(&std::cin == input) )
	delete input;
    return 0;
}
