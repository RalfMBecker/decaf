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
#include "error.h"

extern std::istream* input;

// std::string id_Str;      // for tok_identifier
//long val_Int;            // for tok_Int (int type, not just int)
//double val_Flt;          // for tok_Flt (type)

int lineNo = 1;
int colNo = 0;
int last_Char = ' ';

token*
checkReserved(std::string str)
{
    // misc
    if ( ("eof" == str) )
	return new token();
    if ( ("return" == str) )
	return new token(tok_return);
    // types / pre-defined type values
    if ( ("void" == str) )
	return new token(tok_void);
    if ( ("int" == str) )
	return new token(tok_int);
    if ( ("double" == str) )
	return new token(tok_double);
    if ( ("bool" == str) )
	return new token(tok_bool);
    if ( ("true" == str) )
	return new token(tok_true);
    if ( ("false" == str) )
	return new token(tok_false);
    if ( ("string" == str) )
	return new token(tok_string);
    if ( ("null" == str) )
	return new token(tok_null);
    // looping
    if ( ("for" == str) )
	return new token(tok_for);
    if ( ("while" == str) )
	return new token(tok_while);
    if ( ("if" == str) )
	return new token(tok_if);
    if ( ("else" == str) )
	return new token(tok_else);
    if ( ("break" == str) )
	return new token(tok_break);
    // i/o
    if ( ("Print" == str) )
	return new token(tok_Print);
    if ( ("ReadInteger" == str) )
	return new token(tok_ReadInteger);
    if ( ("ReadLine" == str) )
	return new token(tok_ReadLine);
    // classes
    if ( ("class" == str) )
	return new token(tok_class);
    if ( ("interface" == str) )
	return new token(tok_interface);
    if ( ("this" == str) )
	return new token(tok_this);
    if ( ("extends" == str) )
	return new token(tok_extends);
    if ( ("implements" == str) )
	return new token(tok_implements);
    // heap mgmt
    if ( ("new" == str) )
	return new token(tok_new);
    if ( ("NewArray" == str) )
	return new token(tok_NewArray);

    std::cout << "\t--not a keyword--\n";
    return new token(tok_ID, str);
}

// Some operators are also the first character of a two-char operator - 
// e.g., "<" and "<=". They are checked elsewhere.
token*
retOpPunct(int test)
{
    switch(test){
	//1 char operators
    case '+': return new token(tok_plus); 
    case '-': return new token(tok_minus);
    case '*': return new token(tok_mult);
    case '%': return new token(tok_mod);
    case '.': return new token(tok_dot); 
	/*
    case '/': return new token(tok_div); // handled as part of comments
    case '<': return new token(tok_lt); // handled as part of 2-char operators
    case '>': return new token(tok_gt); // handled as part of 2-char operators
    case '=': return new token(tok_eq); // handled as part of 2-char operators
    case '!': return new token(tok_log_not); // handled as part of 2-char...
    case '[': return new token(tok_sqopen); // handled as part of 2-char...
	*/
	// other punctuation
    case ';': return new token(tok_semi);
    case ',': return new token(tok_comma);
    case ']': return new token(tok_sqclosed);
    case '(': return new token(tok_rdopen);
    case ')': return new token(tok_rdclosed);
    case '{': return new token(tok_paropen);
    case '}': return new token(tok_parclosed);

    default: return new token(tok_eof); // signals: no valid token
    }
}

// cannot use readIntValue as we also keep recording into id_Str here
// errors handled here; returns length of escape sequence if ok
// upon entry, *last points to 'O'/'x' in escape sequence
// upon exit, it points to the last valid number in it
// reports len = -1 if nothing read
int
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
int
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
int
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
int
readIntValue(int* last, int* pbase, int* count, long* iV, std::string& tmp_Str){

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

	*iV = strtol(tmp_Str.c_str(), &end_Ptr, base);
	if ( (tmp_Str.c_str() == end_Ptr) || 
	     ('\0' != end_Ptr[0]) || ( 0 != errno)){
	    if (8 == base)
		tmp_Str.insert(0, "0");
	    else if (16 == base)
		tmp_Str.insert(0, "0x");
	    throw(strtoNumError(tmp_Str, "integer", end_Ptr[0]));
	}
	std::cout << "Found an octal/hex integer literal: " << *iV << "\n";
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
// Note: mem leaks on its own. Relies on caller to delete pointer. 
token*
getTok()
{
    std::string id_Str;      // for tokens with variable values

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

	char *end_Ptr;
	int i = 0, base, ret;
	long iV; // int values get calculated, but returned as string rep
	double fV; // "
	errno = 0;
	std::stringstream tmp_Str;

	base = getBase(&last_Char);

	// calculate number as check for valid numer/possible future use
	ret = readIntValue(&last_Char, &base, &i, &iV, id_Str);
	if ( (tok_int == ret) ){  // if we found an oct/hex int, we are done
	    tmp_Str << iV; // but return as string
	    id_Str = tmp_Str.str();
	    return new token(tok_int, id_Str);	
	}

	if ( ('.' == last_Char) ){ 
	    if ( (MAX_LIT == ++i) )
		throw(tooLongError(id_Str, "MAX_LIT", MAX_LIT));
	    id_Str += last_Char;
	    getNext();
	}
	else{ // we found a dec int, and are done
	    iV = strtol(id_Str.c_str(), &end_Ptr, base);
	    if ( (id_Str.c_str() == end_Ptr) || 
		 ('\0' != end_Ptr[0]) || ( 0 != errno)){
		if (8 == base)
		    id_Str.insert(0, "0");
		else if (16 == base)
		    id_Str.insert(0, "0x");
		throw(strtoNumError(id_Str, "integer", end_Ptr[0]));
	    }
	    tmp_Str << iV;
	    id_Str = tmp_Str.str();
	    std::cout << "Found a decimal integer literal: " << id_Str << "\n";
	    return new token(tok_int, id_Str);	
	}

	// integer after '.'
	while ( isdigit(last_Char) ){
	    if ( (MAX_LIT == ++i) )
		throw(tooLongError(id_Str, "MAX_LIT", MAX_LIT));
	    id_Str += last_Char;
	    getNext();
	}

	// deal with scientific notation
	if ( ('e' == last_Char) || ('E' == last_Char) ){
	    if ( (MAX_LIT == ++i) )
		throw(tooLongError(id_Str, "MAX_LIT", MAX_LIT));
	    id_Str += last_Char;
	    getNext();
	    id_Str += last_Char;
	    if ( !(std::isdigit(last_Char)) && 
		 ('-' != last_Char ) && ('+' != last_Char) )
		throw(Lexer_Error(id_Str, ""));

	    while ( isdigit(getNext())  ){
		if ( (MAX_LIT == ++i) )
		    throw(tooLongError(id_Str, "MAX_LIT", MAX_LIT));
		id_Str += last_Char;
	   }
	}

	fV = strtod(id_Str.c_str(), &end_Ptr);
	if ((id_Str.c_str() == end_Ptr)||('\0' != end_Ptr[0])|| ( 0 != errno) )
	    throw(strtoNumError(id_Str, "float", end_Ptr[0]));
	tmp_Str << fV;
	id_Str = tmp_Str.str();
	std::cout << "Found float literal: " << id_Str << "\n";
	return new token(tok_double, id_Str);
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
	return new token(tok_stringV, id_Str);
    }

    // eat comments
    if ( ('/' == last_Char) ){
	if ('/' == getNext()){
	    while ( ('\n' != getNext()) && (EOF != last_Char) )
		;
	}
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
	    id_Str = last_Char;
	    return new token(tok_div);
	}

	// If we come here, we are at in one of two situations:
	// Comment type 1: pointing to '\n' or EOF at proper comment line
	//         type 2: pointing at the closing tag (last 2 chars '*/).
	std::cout << "\t\t...processed a comment type...\n";
	if ( last_Char != EOF){
	    getNext();
	    return getTok(); // re-throw when done with comment line
	}
	else 
	    return new token(tok_eof);
    }

    // did we hit EOF?
    if ( (EOF == last_Char) )
	return new token(tok_eof);

    // 2 character operator tokens
    if ( ('<' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '<='" << "\n";
	    return new token(tok_le);
	}
	else{
	    std::cout << "found a '<'" << "\n";
	    return new token(tok_lt);
	}
    }
    if ( ('>' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '>='" << "\n";
	    return new token(tok_ge);
	}
	else{
	    std::cout << "found a '>'" << "\n";
	    return new token(tok_gt);
	}
    }
    if ( ('=' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '=='" << "\n";
	    return new token(tok_log_eq);
	}
	else{
	    std::cout << "found a '='" << "\n";
	    return new token(tok_eq);
	}
    }
    if ( ('!' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    std::cout << "found a '!='" << "\n";
	    return new token(tok_log_ne);
	}
	else{
	    std::cout << "found a '!'" << "\n";
	    return new token(tok_log_not);
	}
    }
    if ( ('&' == last_Char) ){
	if ( ('&' == getNext()) ){
	    getNext();
	    std::cout << "found a '&&'" << "\n";
	    return new token(tok_log_and);
	}
	else
	    throw(Lexer_Error("&", ""));
    }
    if ( ('|' == last_Char) ){
	if ( ('|' == getNext()) ){
	    getNext();
	    std::cout << "found a '||'" << "\n";
	    return new token(tok_log_or);
	}
	else
	    throw(Lexer_Error("|", ""));
    }
    if ( ('[' == last_Char) ){
	if ( (']' == getNext()) ){
	    getNext();
	    std::cout << "found a '[]'" << "\n";
	    return new token(tok_sqopenclosed);
	}
	else{
	    std::cout << "found a '['" << "\n";
	    return new token(tok_sqopen);
	}
    }

    // if we come here, we are down to single character operators and 
    // punctuation symbols, and illegal characters. 
    // 'tok_eof' signals: no valid 1-char token; not actually EOF
    // note that we also need to ready the next last_Char
    token* tmpT = retOpPunct(last_Char);
    if ( (tok_eof == tmpT->Tok()) )
	throw(Lexer_Error(std::string(1, last_Char), "invalid token"));
    std::cout << "Found 1-ch OpP: " << tmpT->Lex() << "\n";
    getNext();
    return tmpT;
}
