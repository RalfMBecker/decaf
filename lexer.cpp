/********************************************************************
* lexer.cpp - lexer for Decaf
* 
* Note: space is optional. By and large, lexer reads through it, and 
*       it is the task of the parser to refuse such sequences.
*       However, we forbid the following character sequence types:
*       - 0x1af
*       - 1.1.e
********************************************************************/

#include <string>
#include <sstream>
#include <cctype>
#include "compiler.h"
#include "error.h"
#include "lexer.h"

extern std::istream* input;

std::string identifier_Str;  // for tok_identifier
long val_Int;              // for tok_Int (int type, not just int)
double val_Flt;            // for tok_Flt (type)

static token
checkReserved(std::string word){

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

static void
cError(const char* what, const char* type, int which){
	std::ostringstream tmp_Str;
	tmp_Str << "Lexer: error translating";
	std::cerr << tmp_Str;
	std::cerr << "Lexer: error translating" << what << "to " << type << "type\n";
	exit(EXIT_FAILURE);
}


// Example:
// what_Type="identifier", what=identifier_Str, type_Str="MAX_ID", type=MAX_ID
static void
tooLong(const char* what_Type, const char* what, const char* type_Str,int type){

	std::cerr << "Error: " << what_Type << " (" << what << ") exceeds " 
						<< type_Str << " (" << type << ")\n";
	exit(EXIT_FAILURE);
}

static void
strtolError(const char* str, const char* type, char* end_Ptr, int base){

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


// upon return, points to first digit (if any) of number
static int
getBase(int* last){

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

// gettok - Return the next token from standard input.
int 
getTok() {
  static int last_Char = ' ';

  // eat ws
  while (std::isspace(last_Char))
    last_Char = input->get();

  // legal identifier names are of form [al][alnum]*
  if ( std::isalpha(last_Char) ){ // found an identifier
		int i = 0;
    identifier_Str = last_Char;
		while ( (std::isalnum(last_Char = input->get())) || ('_' == last_Char) ){
			if ( (MAX_ID == ++i) )
				tooLong("Identifier", identifier_Str.c_str(), "MAX_ID", MAX_ID);
			identifier_Str += last_Char;
		}

		std::cout << "Found identifier: " << identifier_Str << "\n";
    return checkReserved(identifier_Str);
  }
 
  // process numbers: [0-9]+([.][0-9]*) (-: NN)
	//                  octal: 0NN; hex: [0xNN, 0XNN]
	//            - unary +/- are processed at end of gettoken()
  //            - we don't allow scientific notation so far
	//            - we allow d., but not .d style of doubles
  if ( std::isdigit(last_Char) ) {
    std::string tmp_Str;
		char *end_Ptr;
		int i = 0, base;
		errno = 0;

		base = getBase(&last_Char);

		if ( (8 == base) || (16 == base) ){
			do{
				if ( (MAX_LIT == ++i) )
					tooLong("Integer literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
				tmp_Str += last_Char;
				last_Char = input->get();
			} while ( ((8==base) && isdigit(last_Char)) || 
								((16==base) && isxdigit(last_Char)) );

			if 	( (16 == base) && (isalpha(last_Char)) ) // catch 0x1ag
				tmp_Str += last_Char;

			val_Int = strtol(tmp_Str.c_str(), &end_Ptr, base);
			if ( ( tmp_Str.c_str() == end_Ptr) || (0 != *end_Ptr) || ( 0 != errno) )
				strtolError(tmp_Str.c_str(), "integer", end_Ptr, base);

			std::cout << "Found integer literal: " << val_Int << "\n";
			return tok_int;
		}

		// dealing with decimal input
		do{
			if ( (MAX_LIT == ++i) )
				tooLong("Integer literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
      tmp_Str += last_Char;
      last_Char = input->get();
    } while (isdigit(last_Char) );

    if ( ('.' == last_Char) ) { 
			if ( (MAX_LIT == ++i) )
				tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
      tmp_Str += last_Char;
      last_Char = input->get();
    }
		else{
			val_Int = strtol(tmp_Str.c_str(), &end_Ptr, base);
			if ( ( tmp_Str.c_str() == end_Ptr) || (0 != *end_Ptr) || ( 0 != errno) )
				strtolError(tmp_Str.c_str(), "integer", end_Ptr, base);

			std::cout << "Found integer literal: " << val_Int << "\n";
			return tok_int;
		}

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
				throw(Lexer_Error(tmp_Str.c_str()));

			while ( isdigit(last_Char = input->get())  ){
				if ( (MAX_LIT == ++i) )
					tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
				tmp_Str += last_Char;
			}
		}

		// TO: CORRECT ERROR CHECKING HERE ********************************
		errno = 0;
    val_Flt = strtod(tmp_Str.c_str(), 0);
		if ( (0 != errno) )
			cError(tmp_Str.c_str(), "float", errno);
		std::cout << "Found float literal: " << val_Flt << "\n";
    return tok_double;
  } // end 'if number' scope

  // eat comments
  if ( ('/' == last_Char) ){
		if ( ('/' == (last_Char = input->get())) ) // comment type 1
			while ( ('\n' != (last_Char = input->get()))  && (EOF != last_Char) )
				;
		else if ( ('*' == (last_Char)) ){ // potential comment type 2
			while ( ('*' != (last_Char = input->get())) && (EOF != last_Char) )
				;
			if ( ('*' == last_Char) ){
				if ( ('/' != (last_Char = input->get())) )
					throw Lexer_Error("/");
			}
			else{
				std::string tmp_String;
				tmp_String += last_Char;
				throw Lexer_Error(tmp_String);
			}
		}
		else{ // found a '/'
			input->putback(last_Char);
			return '/'; // *** RECONSIDER AFTER DEC ON TOKENS
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
