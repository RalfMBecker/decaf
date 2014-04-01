#include "compiler.h"
#include "lexer.h"
#include "driver.h"
#include "error.h"

#include "ast.h"

//void MainLoop();

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

/*
    token* t = getTok();
    word* w = dynamic_cast<word*>(t);
    std::cout << w->Lexeme() << "\n";
*/

    std::cout << "NOTE:\n as currently used, prgm has massive mem leak\n";
    // will be addressed when getTok is incorporated into parser
    while (*input){

	try{
	    while ( (EOF != getTok()->Name() ) )
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
