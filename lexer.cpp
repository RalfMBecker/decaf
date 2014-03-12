/********************************************************************
* lexer.cpp - lexer for Decaf
* 
* Note: space is optional. By and large, lexer reads through it, and 
*       it is the task of the parser to refuse such sequences.
*       However, we forbid the following character sequence types:
*       - 0x1af
*       - 1.1.e
*       We allow C-style string constants syntax
********************************************************************/

#include <string>
#include <sstream>
#include <cctype>
#include "compiler.h"
#include "error.h"
#include "lexer.h"

extern std::istream* input;

std::string id_Str;  // for tok_identifier
long val_Int;              // for tok_Int (int type, not just int)
double val_Flt;            // for tok_Flt (type)

static token
checkReserved(std::string word)
{
    // misc 
    if ( ("eof" == word) )
	return tok_eof;
    if ( ("return" == word) )
	return tok_return;
    // types
    if ( ("void" == word) )
	return tok_void;
    if ( ("int" == word) )
	return tok_int;
    if ( ("double" == word) )
	return tok_double;
    if ( ("bool" == word) )
	return tok_bool;
    if ( ("string" == word) )
	return tok_string;
    if ( ("null" == word) )
	return tok_null;
    // looping
    if ( ("for" == word) )
	return tok_for;
    if ( ("while" == word) )
	return tok_while;
    if ( ("if" == word) )
	return tok_if;
    if ( ("else" == word) )
	return tok_else;
    if ( ("break" == word) )
	return tok_break;
    // i/o
    if ( ("Print" == word) )
	return tok_Print;
    if ( ("ReadInteger" == word) )
	return tok_ReadInteger;
    if ( ("ReadLine" == word) )
	return tok_ReadLine;
    // classes
    if ( ("class" == word) )
	return tok_class;
    if ( ("interface" == word) )
	return tok_interface;
    if ( ("this" == word) )
	return tok_this;
    if ( ("extends" == word) )
	return tok_extends;
    if ( ("implements" == word) )
	return tok_implements;
    // heap mgmt
    if ( ("new" == word) )
	return tok_new;
    if ( ("NewArray" == word) )
	return tok_NewArray;

    std::cout << "\t--not a keyword--\n";
    return tok_ID;
}

// cannot use readIntValue as we also keep recording into the string
// errors handled here; returns length of escape sequence if ok
// upon entry, *last points to 'O'/'x' in escape sequence
// upon exit, it points to the last valid number in it
// reports len = -1 if nothing read
static int
readOctHex(int* last, std::string& Str, int base)
{
    int i = 0;;
    int max = (8 == base)?3:2; 
    std::string test_Str;

    while ( i++ < max ){
	*last = input->get();
	if ( ( (8 == base) && ('0' <= *last) && ('8' > *last) ) || 
	     ( (16 == base) && (std::isxdigit(*last)) ) ){
	    Str += *last;
	    test_Str += *last;
	}
	else{
	    input->putback(*last);
	    break;
	}
    }

    if ( (8 == base) ){ // check if valid ascii (always valid if base = 16)
	char* endPtr;
	long range = strtol(test_Str.c_str(), &endPtr, 8);
	if ( (255 < range) )
	    throw(Lexer_Error(Str, "illegal ascii value"));
    }

    return i-1; // we read one too far
}

// errors handled here; void return if fine
// upon entry, *last point to '\\'. upon exit, to the last valid
// character in the escape sequence
static int
validEscape(int* last, std::string& tmp_Str)
{
    int len = 1;

    *last = input->get();
    switch(*last){

    case 'a': case 'b': case 'f': case 'n': case 'r': case 't':
    case 'v': case '\\': case '\?': case '\'': case '\"':
	tmp_Str += *last;
	break;

    case '0':
	tmp_Str += *last;
	len += readOctHex(last, tmp_Str, 8);
	if ( (1 == len) ) // \0[non-octal]
	    throw(Lexer_Error(tmp_Str, "invalid escape sequence"));
	break;

    case 'x': case 'X':
	tmp_Str += *last; 
	len += readOctHex(last, tmp_Str, 16);
	if ( (1 == len) ) // \x[non-octal]
	    throw(Lexer_Error(tmp_Str, "invalid escape sequence"));
	break;

    default: 
	std::string err_Str;
	err_Str += static_cast<char> (*last);
	throw(Lexer_Error(err_Str, 0));
	break;
    }

    return len;
}


// Example:
// what_Type="identifier", what=id_Str, type_Str="MAX_ID", type=MAX_ID
static void
tooLong(const char* what_Type, const char* what, const char* type_Str,int type)
{

    std::cerr << "Error: " << what_Type << " (" << what << ") exceeds " 
	      << type_Str << " (" << type << ")\n";
    exit(EXIT_FAILURE);
}

static void
strtolError(const char* str, const char* type, char* end_Ptr, int base)
{

    std::ostringstream tmp_Str;
    tmp_Str << "Lexer: error translating ";
    if (8 == base)
	tmp_Str << "0";
    else if (16 == base)
	tmp_Str << "Ox";
    tmp_Str << str << " to " << type;

    // overflow/underflow
    if ( 0 != errno){
	std::cerr << tmp_Str.str() << " - ";
	perror("strtol");
	exit(EXIT_FAILURE);
    }

    if ( (0 != end_Ptr) ){
	tmp_Str << " - offending character: " << *end_Ptr << "\n";
	std::cerr << tmp_Str.str();
	exit(EXIT_FAILURE);
    }
}

static void
strtodError(const char* str, const char* type, char* end_Ptr)
{

    std::ostringstream tmp_Str;
    tmp_Str << "Lexer: error translating " << str << " to " << type;

    // overflow/underflow
    if ( 0 != errno){
	std::cerr << tmp_Str.str() << " - ";
	perror("strtod");
	exit(EXIT_FAILURE);
    }

    if ( (0 != end_Ptr) ){
	tmp_Str << " - offending character: " << *end_Ptr << "\n";
	std::cerr << tmp_Str.str();
	exit(EXIT_FAILURE);
    }
}

// upon return, points to first digit (if any) of number
static int
getBase(int* last)
{

    if ( ('0' != (*last)) )
	return 10;
    else{
	if ( ('x' == ((*last) = input->get())) || ('X' == (*last)) ){
	    *last = input->get();
	    return 16;
	}
	else if ( isdigit(*last) )
	    return 8;
	else{
	    input->putback(*last);
	    *last = '0';
	    return 10;
	}
    }
}

// returns tok_int if octal/hex; 0 if so far correct processing of a dec int
static int
readIntValue(int* last, int* pbase, int* count, std::string& tmp_Str){

    char* end_Ptr;
    int base = *pbase;

    // base = getBase(*last);

    if ( (8 == base) || (16 == base) ){
	do{
	    if ( (MAX_LIT == ++(*count)) )
		tooLong("Integer literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
	    tmp_Str += (*last);
	    (*last) = input->get();
	} while ( ((8==base) && isdigit(*last)) || 
		  ((16==base) && isxdigit(*last)) );

	if ( (16 == base) && (isalpha(*last)) ) // catch 0x1ag
	    tmp_Str += (*last);

	val_Int = strtol(tmp_Str.c_str(), &end_Ptr, base);
	if ( ( tmp_Str.c_str() == end_Ptr) || (0 != *end_Ptr) || ( 0 != errno) )
	    strtolError(tmp_Str.c_str(), "integer", end_Ptr, base);

	std::cout << "Found an octal/hex integer literal: " << val_Int << "\n";
	return tok_int;
    }

    // dealing with decimal input
    do{
	if ( (MAX_LIT == ++(*count)) )
	    tooLong("Integer literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
	tmp_Str += (*last);
	(*last) = input->get();
    } while (isdigit(*last) );
    return 0;
}


// gettok - Return the next token from standard input.
int 
getTok()
{
    static int last_Char = ' ';

    // eat ws
    while (std::isspace(last_Char))
	last_Char = input->get();

    // legal identifier names are of form [al][alnum]*
    if ( std::isalpha(last_Char) ){ // found an identifier
	int i = 0;
	id_Str = last_Char;
	while ( (std::isalnum(last_Char = input->get())) || 
		('_' == last_Char) ){
	    if ( (MAX_ID == ++i) )
		tooLong("Identifier", id_Str.c_str(), "MAX_ID", MAX_ID);
	    id_Str += last_Char;
	}

	std::cout << "Found identifier: " << id_Str << "\n";
	return checkReserved(id_Str);
    }
 
    // process numbers: [0-9]+([.][0-9]*) (-: NN)
    //                  octal: 0NN; hex: [0xNN, 0XNN]
    //                  floats are expected in decimal presentation
    //                  - unary +/- are processed at end of gettoken()
    //                  - we allow d., but not .d style of doubles
    if ( std::isdigit(last_Char) ){
	std::string tmp_Str;
	std::string& rtmp_Str = tmp_Str;
	char *end_Ptr;
	int i = 0, base, ret;
	errno = 0;

	base = getBase(&last_Char);

	ret = readIntValue(&last_Char, &base, &i, rtmp_Str);
	if ( (tok_int == ret) )  // if we found an oct/hex int, we are done
	    return tok_int;	

	if ( ('.' == last_Char) ){ 
	    if ( (MAX_LIT == ++i) )
		tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
	    tmp_Str += last_Char;
	    last_Char = input->get();
	}
	else{ // we found a dec int, and are done
	    val_Int = strtol(tmp_Str.c_str(), &end_Ptr, base);
	    if ( (tmp_Str.c_str() == end_Ptr) || 
		 (0 != *end_Ptr) || (0 != errno) )
		strtolError(tmp_Str.c_str(), "integer", end_Ptr, base);

	    std::cout << "Found a decimal integer literal: " << val_Int << "\n";
	    return tok_int;
	}

	// integer after '.'
	while ( isdigit(last_Char) ){
	    if ( (MAX_LIT == ++i) )
		tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
	    tmp_Str += last_Char;
	    last_Char = input->get();
	}

	// deal with scientific notation
	if ( ('e' == last_Char) || ('E' == last_Char) ){
	    if ( (MAX_LIT == ++i) )
		tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
	    tmp_Str += last_Char;
	    last_Char = input->get();
	    tmp_Str += last_Char;
	    if ( !(std::isdigit(last_Char)) && 
		 ('-' != last_Char ) && ('+' != last_Char) )
		throw(Lexer_Error(tmp_Str, 0));

	    while ( isdigit(last_Char = input->get())  ){
		if ( (MAX_LIT == ++i) )
		    tooLong("Float literal",tmp_Str.c_str(),"MAX_LIT", MAX_LIT);
		tmp_Str += last_Char;
	    }
	}

	val_Flt = strtod(tmp_Str.c_str(), &end_Ptr);
	if ( ( tmp_Str.c_str() == end_Ptr) || (0 != *end_Ptr) || ( 0 != errno) )
	    strtodError(tmp_Str.c_str(), "float", end_Ptr);
	std::cout << "Found float literal: " << val_Flt << "\n";
	return tok_double;
    } // end 'if number' scope

    // process strings
    if ( ('\"' == last_Char) ){	
	int i = 0;
	id_Str.clear();
	while ( ('\"' != (last_Char = input->get())) && (EOF != last_Char)){
	    if ( (MAX_STR == ++i) )
		tooLong("Str const",id_Str.c_str(),"MAX_STR", MAX_STR);
	    if ( ('\n' == last_Char) )
		throw(Lexer_Error(id_Str, "no multi-line strings"));

	    if ( ('\\' == last_Char) ){
		id_Str += '\\';
		i += validEscape(&last_Char, id_Str);
		if ( (MAX_STR < i) )
		    tooLong("Str const",id_Str.c_str(),"MAX_STR",MAX_STR);
	    }
	    else
		id_Str += last_Char;
	}
	std::cout << "Found string const: " << id_Str.c_str() << "\n";
	last_Char = input->get();
	return tok_string;
    }

    // eat comments
    if ( ('/' == last_Char) ){
	if ( ('/' == (last_Char = input->get())) ) // comment type 1
	    while (('\n' != (last_Char = input->get()))  && (EOF != last_Char) )
		;
	else if ( ('*' == (last_Char)) ){ // potential comment type 2
	    while ( ('*' != (last_Char = input->get())) && (EOF != last_Char) )
		;
	    if ( ('*' == last_Char) ){
		if ( ('/' != (last_Char = input->get())) )
		    throw Lexer_Error("/", 0);
	    }
	    else{
		std::string tmp_String;
		tmp_String += last_Char;
		throw Lexer_Error(tmp_String, 0);
	    }
	}
	else{ // found a '/'
	    input->putback(last_Char);
	    return '/';
	}

	std::cout << "\t\t...processed a comment type...\n";
	if ( last_Char != EOF)
	    return getTok(); // re-throw when done with comment line
    }

    // did we hit EOF?
    if ( (EOF == last_Char) )
	return tok_eof;
 
    // if we come here, we have found no token etc, and return the char we found
    // note that we also need to ready the next last_Char
    int tmp_Char = last_Char;
    last_Char = input->get();
    std::cout << "Found character: " << static_cast<char> (tmp_Char) << "\n";
    return tmp_Char;
}
