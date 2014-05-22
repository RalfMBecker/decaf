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

// Entry:       should point to " (caller to ensure it's not escape sequence \")
// Exit:        points to terminating " (or to eof, if none found)
// Rv:          -1 - eof; 0 - found " " delimited string
// Arg ret_Str: write characters found to end of ret_Str
// Errors:      done by lexer (e.g., accept \n and other escape sequences)
// Note:        reads in illegal-in-string '\n' char - lexer to handle
int
getString(std::string& ret_Str)
{
    char c;

    while ( (EOF != (c = input->get())) && ('\"' != c) ){
	ret_Str += c;
	if ( ('\\' == c) ){ // print 2 characters at a time
	    if ( (EOF == (c = input->get())) ){
		break;
	    }
	    ret_Str += c;
	}
    }

    return (EOF == c)?-1:0;
}

// get next non-ws character, and count up 'count' for each '\n' found
char
getNoWs(std::fstream* stream_O, int& count)
{
    char c;

    while ( std::isspace(c = stream_O->get()) ){
	if ( (EOF == c) ) break; // for safety; unnecessary
	else if ( ('\n' == c) ) count++;
    }

    return c;
}

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

	// If we see an '\', print 2 chars at a time (to ensure that when
	// we see a '\"' below, it's not an escape sequence, but a real string)
	else if ( ('\\' == c) ){
	    file_Preproc->put(c);
	    if ( (EOF == (c = input->get())) )
		break;
	    file_Preproc->put(c);
	}

	// concatenate adjacent strings
	else if ( ('\"' == c) ){

	    std::string tmp_Str; // cumulative new string content
	    int count = 0;
	    while ( ('\"' == c) ){
		if ( (-1 == getString(tmp_Str)) )
		    break; 
		c = getNoWs(input, count); // read in an adjacent opening "
	    }
	    if ( (EOF != c) )
		input->putback(c);

	    std::string out_Str = "\"";
	    out_Str += tmp_Str;
	    out_Str += "\"";
	    for (int i = 0; i < count; i++)
		out_Str += "\n";
	    file_Preproc->write(out_Str.c_str(), out_Str.size());
	}

	else // character not currently especially handled
	    file_Preproc->put(c); // no whitespace removal
    }

deep_Jump:
    file_Preproc->put('\n');
    file_Preproc->flush(); // as we don't delete the pointer in this function

    file_Preproc->seekg(0, file_Preproc->beg); // won't put to it, so not reset

    input = file_Preproc;
}
