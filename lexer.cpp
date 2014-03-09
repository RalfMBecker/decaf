/********************************************************************
* lexer.cpp - lexer for Decaf
* 
********************************************************************/

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
cError(const char* what, const char* type){
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
		while ( (std::isalnum(last_Char = input->get())) ){
			if ( (MAX_ID == ++i) )
				tooLong("Identifier", identifier_Str.c_str(), "MAX_ID", MAX_ID);
			identifier_Str += last_Char;
		}

		std::cout << "Found identifier: " << identifier_Str << "\n";
    return checkReserved(identifier_Str);
  }
 
  // process numbers: [0-9]+([.][0-9]*)
	//            - unary +/- are processed at end of gettoken()
  //            - we don't allow scientific notation so far
	//            - we allow d., but not .d style of doubles
	//            - extension to read in octal/hex easy
  if ( std::isdigit(last_Char) ) {
    std::string tmp_Str;
		int i = 0;
		do{
			if ( (MAX_LIT == ++i) )
				tooLong("Integer literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
      tmp_Str += last_Char;
      last_Char = input->get();
    } while ( isdigit(last_Char) );

    if ( ('.' == last_Char) ) { 
			if ( (MAX_LIT == ++i) )
				tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
      tmp_Str += last_Char;
      last_Char = input->get();
    }
		else{
			errno = 0;
			val_Int = strtol(tmp_Str.c_str(), 0, 10);
			if ( (0 != errno) ) 
				cError(tmp_Str.c_str(), "integer");
			std::cout << "Found integer literal: " << val_Int << "\n";
			return tok_int;
		}

    while ( isdigit(last_Char) ){
			if ( (MAX_LIT == ++i) )
				tooLong("Float literal", tmp_Str.c_str(), "MAX_LIT", MAX_LIT);
      tmp_Str += last_Char;
      last_Char = input->get();
    }
		errno = 0;
    val_Flt = strtod(tmp_Str.c_str(), 0);
		if ( (0 != errno) )
			cError(tmp_Str.c_str(), "float");
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
