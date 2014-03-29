/********************************************************************
* lexer.cpp - lexer for Decaf
* 
*       All Decaf features are covered (or believed to be). Addtl., 
*       we allow C-style string constant syntax, octal integers,
*       and a few other constructs that are not expected of Decaf.
*
*       Lexer can be relied upon to perform lexer-level syntax checks
*
* Note: space is optional. By and large, lexer reads through it, and 
*       it is the task of the parser to refuse such sequences.
*       However, we forbid the following character sequence types:
*       - 0x1af
*       - 1.1.e
*       (this is as specified in the Brown grammer)
*
********************************************************************/

#include <string>
#include <sstream>
#include <cctype>
#include "compiler.h"
#include "lexer.h"

extern std::istream* input;

std::string id_Str;      // for tok_identifier
long val_Int;            // for tok_Int (int type, not just int)
double val_Flt;          // for tok_Flt (type)

int lineNo = 1;
int colNo = 0;
int last_Char = ' ';

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
    if ( ("true" == word) )
	return tok_true;
    if ( ("false" == word) )
	return tok_false;
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

// returns 0 if character in dictionary; -1 if not
// as some operators consist of 2 characters, they are checked elsewhere
// some duplication with prior checks (e.g., '<') (for safety in case
// of later re-design)
static int
checkValidOpPunct(int test)
{
    if ( ('+' == test) || ('-' == test) || ('*' == test) || ('/' == test) || 
	 ('%' == test) || ('<' == test) || ('>' == test) || ('=' == test) ||
	 ('!' == test) || (';' == test) || (',' == test) || ('.' == test) ||
	 ('[' == test) || (']' == test) || ('(' == test) || (')' == test) ||
	 ('{' == test) || ('}' == test) )
	return 0;
    else
	return -1;
}

// cannot use readIntValue as we also keep recording into id_Str here
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

// errors handled here; returns lenght of escape sequence if fine
// upon entry, *last point to '\\'. 
// upon exit, it points to the last valid character in the escape sequence
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
	throw(Lexer_Error(err_Str, ""));
	break;
    }

    return len;
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
// (handled differently as octal/hex can only be integers; but for decimals,
// we might have parsed only the 'x' part of x.y[e[+|-]z]] )
static int
readIntValue(int* last, int* pbase, int* count, std::string& tmp_Str){

    char* end_Ptr;
    int base = *pbase;

    if ( (8 == base) || (16 == base) ){
	do{
	    if ( (MAX_LIT == ++(*count)) )
		throw(tooLongError(tmp_Str, "MAX_LIT", MAX_LIT));
	    tmp_Str += (*last);
	    (*last) = input->get();
	} while ( ((8==base) && isdigit(*last)) || 
		  ((16==base) && isxdigit(*last)) );

	if ( (16 == base) && (isalpha(*last)) ) // catch 0x1ag
	    tmp_Str += (*last);

	val_Int = strtol(tmp_Str.c_str(), &end_Ptr, base);
	if ( (tmp_Str.c_str() == end_Ptr) || 
	     ('\0' != end_Ptr[0]) || ( 0 != errno)){
	    if (8 == base)
		tmp_Str.insert(0, "0");
	    else if (16 == base)
		tmp_Str.insert(0, "0x");
	    throw(strtoNumError(tmp_Str, "integer", end_Ptr[0]));
	}
	std::cout << "Found an octal/hex integer literal: " << val_Int << "\n";
	return tok_int;
    }

    // dealing with decimal input
    do{
	if ( (MAX_LIT == ++(*count)) )
	    throw(tooLongError(tmp_Str, "MAX_LIT", MAX_LIT));
	tmp_Str += (*last);
	(*last) = input->get();
    } while (isdigit(*last) );
    return 0;
}

int
getNext(void)
{
    // do this first: if we pointed to '\n' when error was found, necessary
    if ( ('\n' == last_Char) ){
	lineNo++;
	colNo = 1;
    }
    else
	colNo++;

    return (last_Char = input->get());
}

// gettok - return the next token from 'input'-stream
// invariant: upon return (other than from EOF), last_Char has the next
//            unprocessed char
int 
getTok()
{
    // eat ws
    while (std::isspace(last_Char))
	getNext();

    // legal identifier names are of form [al][alnum]*
    if ( std::isalpha(last_Char) ){ // found an identifier
	int i = 0;
	id_Str = last_Char;
	while ( (std::isalnum(getNext())) || 
		('_' == last_Char) ){
	    if ( (MAX_ID == ++i) )
		throw(tooLongError(id_Str, "MAX_ID", MAX_ID));
	    id_Str += last_Char;
	}

	std::cout << "Found identifier: " << id_Str << "\n";
	return checkReserved(id_Str);
    }
 
    // process numbers: [0-9]+([.][0-9]*) (=: NN)
    //                  octal: 0NN; hex: [0xNN, 0XNN]
    //                  floats are expected in decimal presentation
    //                  - unary +/- are processed at end of gettoken()
    //                  - we allow d., but not .d style of doubles
    //                    (as also specified in Decaf grammar)
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
		throw(tooLongError(tmp_Str, "MAX_LIT", MAX_LIT));
	    tmp_Str += last_Char;
	    getNext();
	}
	else{ // we found a dec int, and are done
	    val_Int = strtol(tmp_Str.c_str(), &end_Ptr, base);
	    if ( (tmp_Str.c_str() == end_Ptr) || 
		 ('\0' != end_Ptr[0]) || ( 0 != errno)){
		if (8 == base)
		    tmp_Str.insert(0, "0");
		else if (16 == base)
		    tmp_Str.insert(0, "0x");
		throw(strtoNumError(tmp_Str, "integer", end_Ptr[0]));
	    }
	    std::cout << "Found a decimal integer literal: " << val_Int << "\n";
	    return tok_int;
	}

	// integer after '.'
	while ( isdigit(last_Char) ){
	    if ( (MAX_LIT == ++i) )
		throw(tooLongError(tmp_Str, "MAX_LIT", MAX_LIT));
	    tmp_Str += last_Char;
	    getNext();
	}

	// deal with scientific notation
	if ( ('e' == last_Char) || ('E' == last_Char) ){
	    if ( (MAX_LIT == ++i) )
		throw(tooLongError(tmp_Str, "MAX_LIT", MAX_LIT));
	    tmp_Str += last_Char;
	    getNext();
	    tmp_Str += last_Char;
	    if ( !(std::isdigit(last_Char)) && 
		 ('-' != last_Char ) && ('+' != last_Char) )
		throw(Lexer_Error(tmp_Str, ""));

	    while ( isdigit(getNext())  ){
		if ( (MAX_LIT == ++i) )
		    throw(tooLongError(tmp_Str, "MAX_LIT", MAX_LIT));
		tmp_Str += last_Char;
	   }
	}

	val_Flt = strtod(tmp_Str.c_str(), &end_Ptr);
	if ((tmp_Str.c_str() == end_Ptr)||('\0' != end_Ptr[0])|| ( 0 != errno) )
	    throw(strtoNumError(tmp_Str, "float", end_Ptr[0]));
	std::cout << "Found float literal: " << val_Flt << "\n";
	return tok_double;
    } // end 'if number' scope

    // process strings
    if ( ('\"' == last_Char) ){	
	int i = 0;
	id_Str.clear();
	while ( ('\"' != getNext()) && (EOF != last_Char)){
	    if ( (MAX_STR == ++i) )
		throw(tooLongError(id_Str, "MAX_STR", MAX_STR));
	    else if ( ('\n' == last_Char) || (EOF == last_Char) )
		throw(Lexer_Error(id_Str, "string missing closing \""));
	    else if ( ('\\' == last_Char) ){
		id_Str += '\\';
		i += validEscape(&last_Char, id_Str);
		if ( (MAX_STR < i) )
		    throw(tooLongError(id_Str, "MAX_STR", MAX_STR));
	    }
	    else
		id_Str += last_Char;
	}
	std::cout << "Found string const: " << id_Str.c_str() << "\n";
	getNext();
	return tok_string;
    }

    // eat comments
    if ( ('/' == last_Char) ){
	if ('/' == getNext())
	    while ( ('\n' != getNext()) && (EOF != last_Char) )
		;
	else if ( ('*' == last_Char) ){ // potential comment type 2
	    id_Str = "/*";
	    for (;;){ // need infinite loop to allow for /* * */ type 
		id_Str += getNext();
		if ( (EOF == last_Char) )
		    throw(Lexer_Error(id_Str, "comment missing closing */"));
		else if ( ('*' == last_Char) ){
		    if ( ('/' == getNext()) ){
			id_Str.clear();
			break; // found a type 2 comment
		    }
		    else
			input->putback(static_cast<char>(last_Char));
		}
	    }
	} // end loop for type 2 comments
	else{ // found a '/' char
	    std::cout << "found a /\n";
	    return '/';
	}

	// If we come here, we are at in one of two situations:
	// Comment type 1: pointing to '\n' or EOF at proper comment line
	//         type 2: pointing at the closing tag (last 2 chars '*/).
	std::cout << "\t\t...processed a comment type...\n";
	if ( last_Char != EOF)
	    return getTok(); // re-throw when done with comment line
	else 
	    return tok_eof;
    }

    // did we hit EOF?
    if ( (EOF == last_Char) )
	return tok_eof;

    // 2 character operator tokens
    if ( ('<' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '<='" << "\n";
	    return tok_se;
	}
	else{
	    std::cout << "found a '<'" << "\n";
	    return '<';
	}
    }
    if ( ('>' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '>='" << "\n";
	    return tok_ge;
	}
	else{
	    std::cout << "found a '>'" << "\n";
	    return '>';
	}
    }
    if ( ('=' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '=='" << "\n";
	    return tok_log_eq;
	}
	else{
	    std::cout << "found a '='" << "\n";
	    return '=';
	}
    }
    if ( ('!' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '!='" << "\n";
	    return tok_log_ne;
	}
	else
	    throw(Lexer_Error("!", ""));
    }
    if ( ('&' == last_Char) ){
	if ( ('&' == getNext()) ){
	    getNext();
	    std::cout << "found a '&&'" << "\n";
	    return tok_log_and;
	}
	else
	    throw(Lexer_Error("&", ""));
    }
    if ( ('|' == last_Char) ){
	if ( ('|' == getNext()) ){
	    getNext();
	    std::cout << "found a '||'" << "\n";
	    return tok_log_or;
	}
	else
	    throw(Lexer_Error("|", ""));
    }
    if ( ('[' == last_Char) ){
	if ( (']' == getNext()) ){
	    getNext();
	    std::cout << "found a '[]'" << "\n";
	    return tok_sqbrack_closed;
	}
	else{
	    std::cout << "found a '['" << "\n";
	    return '[';
	}
    }

    // if we come here, we are down to single character operators and 
    // punctuation symbols, and illegal characters. 
    // for legal characters, we return their ascii value
    // note that we also need to ready the next last_Char
    if ( (-1 == checkValidOpPunct(last_Char)) ){
	std::string tmp;
	tmp += static_cast<char> (last_Char);
	throw(Lexer_Error(tmp, ""));
    }
    int tmp_Char = last_Char;
    getNext();
    std::cout << "Found character: " << static_cast<char> (tmp_Char) << "\n";
    return tmp_Char;
}
