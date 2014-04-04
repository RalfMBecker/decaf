#include "compiler.h"
#include "lexer.h"
#include "driver.h"
#include "error.h"

#include "ast.h" // REMOVE - just to keep an eye on it
#include "tables.h" // REMOVE - just to keep an eye on it

std::istream* input;

int
main(int argc, char* argv[])
{
    // testing symbol table mgm 
    Env* root_Env = makeEnvRoot();
    Env* e1 = addEnv(root_Env);
    Env* e2 = addEnv(e1);
    Env* e3 = addEnv(e2);

    std::cout << "Table level info\n";
    std::cout << "---------------------------------\n";
    std::string tabName1 = e1->getTableName();
    std::string tabName2 = e2->getTableName();
    std::string tabName3 = e3->getTableName();

    std::cout << "tabName1 = " << tabName1 << "\n";
    std::cout << "tabName2 = " << tabName2 << "\n";
    std::cout << "tabName3 = " << tabName3 << "\n\n";

    std::cout << "(e3->getPrior()).getTableName() = " << 
	(e3->getPrior())->getTableName() << "\n\n";

    std::cout << tabName1 << ": offsetHeap_ = " << ST[tabName1].getOffsetHeap();
    std::cout << "\toffsetStack_ = " << ST[tabName1].getOffsetStack() << "\n";
    std::cout << tabName2 << ": offsetHeap_ = " << ST[tabName2].getOffsetHeap();
    std::cout << "\toffsetStack_ = " << ST[tabName2].getOffsetStack() << "\n";
    std::cout << tabName3 << ": offsetHeap_ = " << ST[tabName3].getOffsetHeap();
    std::cout << "\toffsetStack_ = " << ST[tabName3].getOffsetStack() << "\n";

    std::cout << "\nPrinting ancestor tables of e3:\n";
    printEnvAncestorInfo(e3);
    std::cout << "Printing entries in ST:\n";
    printSTInfo();

    std::cout << "\nTable entry info\n";
    std::cout << "---------------------------------\n";
    if ( (-1 == addEnvName(e1, "ralf", "int", "heap", 8)) )
	std::cout << "error entering ralf into " << tabName1 << "\n";
    std::cout << "e1: added ralf (int) to heap\n";
    if ( (-1 == addEnvName(e2, "fluff", "int", "heap", 8)) )
	std::cout << "error entering fluff into " << tabName2 << "\n";
    std::cout << "e2: added fluff (int) to heap\n";
    if ( (-1 == addEnvName(e2, "octo", "float", "stack", 16)) )
	std::cout << "error entering octo into " << tabName2 << "\n";
    std::cout << "e2: added octo (float) to stack\n";
    if ( (-1 == addEnvName(e3, "JL", "class", "heap", 64)) )
	std::cout << "error entering JL into " << tabName3 << "\n";
    std::cout << "e3: added JL (class - 64) to heap\n";
    if ( (-1 == addEnvName(e3, "ralf", "int", "heap", 8)) )
	std::cout << "error entering ralf into " << tabName3 << "\n";
    std::cout << "e3: added ralf (int) to heap\n";

    std::cout << "\noffsetHeap_ and offsetStack_ are now:\n";
    std::cout << tabName1 << ": offsetHeap_ = " << ST[tabName1].getOffsetHeap();
    std::cout << "\toffsetStack_ = " << ST[tabName1].getOffsetStack() << "\n";
    std::cout << tabName2 << ": offsetHeap_ = " << ST[tabName2].getOffsetHeap();
    std::cout << "\toffsetStack_ = " << ST[tabName2].getOffsetStack() << "\n";
    std::cout << tabName3 << ": offsetHeap_ = " << ST[tabName3].getOffsetHeap();
    std::cout << "\toffsetStack_ = " << ST[tabName3].getOffsetStack() << "\n";

    std::cout << "\nPrinting ancestor tables of e3:\n";
    printEnvAncestorInfo(e3);
    std::cout << "Printing entries in ST:\n";
    printSTInfo();

    return 0;

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
    makeEnvRoot();

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
