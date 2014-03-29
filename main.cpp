#include "compiler.h"

//void MainLoop();

#include "lexer.h"

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
    std::cout << "ready> ";
    // getNextToken();

    while (*input){

	try{
	    while ( (EOF != getTok() ) )
		;
	}

	catch(Error& m){
	    m.print();
	}
    }

    if ( !(&std::cin == input) )
	delete input;
    return 0;
}
