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

extern int option_Debug;
extern int option_Preproc;
extern int option_IR;
extern int option_OptLevel;

extern std::string base_Name;
extern std::fstream* input;
extern std::fstream* file_Preproc;
extern std::fstream* file_IR;

int
main(int argc, char* argv[])
{
    int opt;
    char* pArg;
    std::string err = "unexpected error while processing command line options";
    std::string opt_Str = ":dpiO:"; 

    while ( (-1 != (opt = getopt(argc, argv, opt_Str.c_str()))) ){
	if ( ('?' == opt) || (':' == opt) ){
	    std::cerr << argv[0] << ": error - invalid option - ";
	    std::cerr << "offending character \'";
	    std::cerr << static_cast<char>(optopt) << "\'\n";
	    usageErr(argv[0]);
	}

	switch(opt){
	case 'd': option_Debug = 1; break;
	case 'p': option_Preproc = 1; break; // ** TO DO: better
	case 'i': option_IR = 1; option_Preproc = 0; break;
	case 'O': 
	    pArg = optarg;
	    if ( (0 == strcmp(pArg, "0")) )
		option_OptLevel = 1;
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

    input = new std::fstream(name_Str.c_str());
    if ( !(input->good()) )
	errExit(1, "%s: can't open file <%s>", argv[0], argv[optind]);

    // relegate execution to a driver module
    preProcess(name_Str);
    if ( !(option_Preproc) ){
	std::streambuf* cout_Buf;
	input = file_Preproc;

	if (option_IR){
	    std::string name_Str = base_Name + ".ir";
	    std::fstream::openmode o_M = std::fstream::in | std::fstream::out;
	    o_M |= std::fstream::trunc;

	    file_IR = new std::fstream(name_Str.c_str(), o_M);
	    if ( !(file_IR->good()) )
		errExit(1, "can't open file <%s>", name_Str.c_str());
	    cout_Buf = std::cout.rdbuf(); // save old buffer
	    std::cout.rdbuf(file_IR->rdbuf());

	}

	initFrontEnd(name_Str);
	collectParts();
	startParse();
	astToIR();

	if (option_IR){
	    file_IR->flush();

	    std::cout.rdbuf(cout_Buf); // reset to old buffer
	}

	cleanUp();
    }
    else{
	delete input;
	delete file_Preproc;
    }

    exit(EXIT_SUCCESS);
}
