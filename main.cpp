/********************************************************************
* main.cpp - program entry point
*
* Conventions:
*
* Include order: lexer.h, ast.h, error.h, tables.h, parser.h, 
*                ir.h, visitor.h, driver.h
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

#include <string>
#include <cstdarg>
#include <cstring>

#include "compiler.h"
#include "driver.h"

// forward declaration
void errExit(int pError, const char* msg, ...);

extern std::istream* input;
extern int option_Debug;

int
main(int argc, char* argv[])
{
    std::string name_Str;
    switch(argc){
/*    case 1:
	input = &std::cin;
	name_Str = "std::cin";
	break; */
    case 2: 
    case 3:
	input = new std::ifstream(argv[1]);
	if ( !(input->good()) )
	    errExit(1, "%s: can't open file <%s>", argv[0], argv[1]);
	name_Str = argv[1];

	if ( (3 == argc) ){ // ** TO DO: extend on options (meh as is)
	    if ( (0 == strcmp("-d", argv[2])) )
		option_Debug = 1;
	}
	break;
    default:
	errExit(0, "Error: # of args (%d). Usage: <program> <file>", argc);
    }

    // relegate execution to a driver module
    initFrontEnd(name_Str);
    collectParts();
    startParse();

/*    if ( !(&std::cin == input) )
	delete input; */
    return 0;
}
