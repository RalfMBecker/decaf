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
void usageErr(std::string);

extern std::istream* input;
extern int option_Debug;
extern int option_optLevel;

int
main(int argc, char* argv[])
{
    int opt;
    char* pArg;
    std::string err = "unexpected error while processing command line options";
    std::string opt_Str = ":dO:"; 

    while ( (-1 != (opt = getopt(argc, argv, opt_Str.c_str()))) ){
	if ( ('?' == opt) || (':' == opt) ){
	    std::cerr << argv[0] << ": error - invalid option - ";
	    std::cerr << "offending character \'";
	    std::cerr << static_cast<char>(optopt) << "\'\n";
	    usageErr(argv[0]);
	}

	switch(opt){
	case 'd': option_Debug = 1; break;
	case 'O': 
	    pArg = optarg;
	    if ( (0 == strcmp(pArg, "0")) )
		option_optLevel = 1;
	    else
		errExit(0, "invalid optimization level %s", pArg);
	    break;
	default: 
	    errExit(0, err.c_str());
	    break;
	}
    }

    if ( (0 == argv[optind]) )
	errExit(0, "%s: Error - no file to compile specified", argv[0]);
    std::string name_Str = argv[optind];
    if ( 5 > name_Str.size() )
	errExit(0, "%s: Error - invalid filename", argv[0]);
    size_t pos = name_Str.size() - 4;
    std::string ext_Str = name_Str.substr(pos, 4);
    if ( 0 != strcmp(".dec", ext_Str.c_str()) ){
	std::string err_FmtStr = "%s: Error - invalid file extension (%s)"; 
	errExit(0, err_FmtStr.c_str(), argv[0], ext_Str.c_str());
    }

    input = new std::ifstream(name_Str.c_str());
    if ( !(input->good()) )
	errExit(1, "%s: can't open file <%s>", argv[0], argv[optind]);

    // relegate execution to a driver module
    initFrontEnd(name_Str);
    collectParts();
    startParse();

    cleanUp();
    delete input;
    exit(EXIT_SUCCESS);
}
