/********************************************************************
* preproc.cpp - preprocessor for Decaf
*
* Currently does:
* - strip comments
* - concatenate adjacent strings
* - add a "\n" at EOF
*
* Error checking: none (job of the lexer), except for comments
*                 as they are removed here
*
* Challenge: to ensure that line and column numbers can be extracted
*            by lexer such that they match the source file (for 
*            meaningful error reporting)
*
********************************************************************/

#include <sstream>
#include <fstream>
#include <string>
#include <string.h> // for basename()
#include <stdarg.h>
#include "lexer.h"

// forward declaration
void errExit(int pError, const char* msg, ...);

extern std::string base_Name;
extern std::fstream* input;
extern std::fstream* file_Source;
extern std::fstream* file_Preproc;

void
preProcess(std::string In_Name)
{
    file_Source = input;

    std::string tmp_Name = basename(In_Name.c_str());
    size_t pos = tmp_Name.size() - 4; // error checking in main.cpp
    base_Name = tmp_Name.substr(0, pos);
    std::string out_Name = base_Name + ".pre"; // write to cwd

    std::fstream::openmode o_M = std::fstream::in | std::fstream::out;
    o_M |= std::fstream::trunc;
    file_Preproc = new std::fstream(out_Name.c_str(), o_M);
    if ( !(file_Preproc->good()) )
	errExit(1, "can't open file <%s>", out_Name.c_str());

    char c;
    while ( (EOF != (c = input->get())) ){
	if ( ('/' == c) ){
	    if ( ('/' == (c = input->get())) ){
		while ( (EOF != (c = input->get())) && ('\n' != c) )
		    ;
		file_Preproc->put('\n');
	    }
	    else if ( ('*' == c) ){ // comment type 2
		std::string tmp_Str = "/*";
		std::string e_Msg = "Error: reached end of file while ";
		e_Msg += "processing comment type 2 (missing */)\n";
		int count = 0;
		for (;;){ // need infinite loop to allow for /* * */ type 
		    if ( (EOF == (c = input->get())) ){
			std::cerr << e_Msg;
			goto deep_Jump;
		    }
		    else if ( ('\n' == c) )
			count++;
		    tmp_Str += c;  

		    if ( ('*' == c) ){
			if ( ('/' == (c = input->get())) ){
			    tmp_Str = "";
			    for (int i = 0; i < count; i++)
				tmp_Str += "\n";
			    //int size = tmp_Str.size();
			    if ( (0 < count) )
				file_Preproc->write(tmp_Str.c_str(), count);
			    break; 
			}
			else{
			    if ( (EOF == c) ){
				std::cerr << e_Msg;
				goto deep_Jump;
			    }
			    input->putback(static_cast<char>(c));
			}
		    }
		} // end infinite loop
	    } // end type 2 comments
	    else{ // found a '/'
		file_Preproc->put('/');
		input->putback(static_cast<char>(c));
	    }
	}
	else // regular character
	    file_Preproc->put(c); // no whitespace removal
    }

deep_Jump:
    file_Preproc->put('\n');
    file_Preproc->flush(); // as we don't delete the pointer in this function

    file_Preproc->seekg(0, file_Preproc->beg); // won't put to it, so not reset

    // ** TO DO: string concatenation. set input to file_Preproc first,
    //           and do the flushing seeking etc twice

    input = file_Preproc;
}
