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
*       (this is as specified in the Brown grammar)
*
* Error handling: propagated up to parser level (see there)
*
********************************************************************/

#include <string>
#include <sstream>
#include <cctype>

#include "compiler.h"
#include "lexer.h"
#include "error.h"

extern int option_Debug;
extern std::fstream* input;

int line_No = 1;
int col_No = 0;
int last_Char = ' ';
int errorIn_Progress = 0;

token next_Token;

int
getNext(void)
{
    // do this first: if we pointed to '\n' when error was found, necessary
    if ( ('\n' == last_Char) ){
	line_No++;
	col_No = 0;
    }
    else
	col_No++;

    return (last_Char = input->get());
}

// To wrap back around lines, we would need to track chars on a stack
// This scheme might create phantom col numbers, but rarely used. 
void
putBack(char c)
{ 
    col_No--;
    input->putback(c);
}

token
getNextToken(void) { return (next_Token = getTok()); }

token
checkReserved(std::string Str)
{
    // misc
    if ( ("eof" == Str) )
	return token();
    if ( ("return" == Str) )
	return token(tok_return);
    // types / pre-defined type values
    if ( ("void" == Str) )
	return token(tok_void);
    if ( ("int" == Str) )
	return token(tok_int);
    if ( ("double" == Str) )
	return token(tok_double);
    if ( ("bool" == Str) )
	return token(tok_bool);
    if ( ("true" == Str) )
	return token(tok_true);
    if ( ("false" == Str) )
	return token(tok_false);
    if ( ("string" == Str) )
	return token(tok_string);
    if ( ("null" == Str) )
	return token(tok_null);
    // looping
    if ( ("for" == Str) )
	return token(tok_for);
    if ( ("while" == Str) )
	return token(tok_while);
    if ( ("if" == Str) )
	return token(tok_if);
    if ( ("else" == Str) )
	return token(tok_else);
    if ( ("break" == Str) )
	return token(tok_break);
    if ( ("continue" == Str) )
	return token(tok_cont);
    // i/o
    if ( ("Print" == Str) )
	return token(tok_Print);
    if ( ("ReadInteger" == Str) )
	return token(tok_ReadInteger);
    if ( ("ReadLine" == Str) )
	return token(tok_ReadLine);
    // classes
    if ( ("class" == Str) )
	return token(tok_class);
    if ( ("interface" == Str) )
	return token(tok_interface);
    if ( ("this" == Str) )
	return token(tok_this);
    if ( ("extends" == Str) )
	return token(tok_extends);
    if ( ("implements" == Str) )
	return token(tok_implements);
    // heap mgmt
    if ( ("new" == Str) )
	return token(tok_new);
    if ( ("NewArray" == Str) )
	return token(tok_NewArray);

    // ** TO DO: probably can be removed
    // preprocessing tokens:
    // - currently only from removing comments and concatenating strings 
    //   expected syntax: linup__ <unsigned integer>"\n"
    if ( ("lineup__" == Str) ){
	int c;
	while (std::isspace(c = input->get()))
	    ;
	if ( !(std::isdigit(c) ) )
	    errExit(0, "preprocessor logic error");

	std::string tmp_String;
	do{
	    tmp_String += c;
	} while (std::isdigit(c = input->get()));

	line_No += atoi(tmp_String.c_str());
	int TTT = atoi(tmp_String.c_str());
	std::cout << "\t\t\t\tadded lines: " << TTT << "\n"; 

	col_No = 0;
	input->get(); // to not add a false line to count
	return getNextToken();
    }

    return token(tok_ID, Str);
}

// Some operators are also the first character of a two-char operator - 
// e.g., "<" and "<=". They are checked elsewhere.
token
retOpPunct(int Test)
{
    switch(Test){
	//1 char operators
    case '+': return token(tok_plus); 
    case '-': return token(tok_minus);
    case '*': return token(tok_mult);
    case '%': return token(tok_mod);
    case '.': return token(tok_dot); 
	/*
    case '/': return token(tok_div); // handled as part of comments
    case '<': return token(tok_lt); // handled as part of 2-char operators
    case '>': return token(tok_gt); // handled as part of 2-char operators
    case '=': return token(tok_eq); // handled as part of 2-char operators
    case '!': return token(tok_log_not); // handled as part of 2-char...
    case '[': return token(tok_sqopen); // handled as part of 2-char...
	*/
	// other punctuation
    case ';': return token(tok_semi);
    case ',': return token(tok_comma);
    case ']': return token(tok_sqclosed);
    case '(': return token(tok_rdopen);
    case ')': return token(tok_rdclosed);
    case '{': return token(tok_paropen);
    case '}': return token(tok_parclosed);

    default: 
	errorIn_Progress = 1;
	return token(tok_err);
    }
}

// Cannot use readIntValue as we also keep recording into id_Str here
// Returns: length of escape sequence if ok; -1 if nothing read; 
//          -2 if error
// Upon entry, *last points to 'O'/'x' in escape sequence
// Upon exit, it points to the last valid number in it
int
readOctHex(int* Last, std::string& Str, int Base)
{
    int i = 0;;
    int max = (8 == Base)?3:2; 
    std::string test_Str;

    while ( i++ < max ){
	*Last = getNext();
	if ( ( (8 == Base) && ('0' <= *Last) && ('8' > *Last) ) || 
	     ( (16 == Base) && (std::isxdigit(*Last)) ) ){
	    Str += *Last;
	    test_Str += *Last;
	}
	else{
	    putBack(*Last);
	    break;
	}
    }

    if ( (8 == Base) ){ // check if valid ascii (always valid if base = 16)
	char* endPtr;
	long range = strtol(test_Str.c_str(), &endPtr, 8);
	if ( (255 < range) ){
	    lexerError(0, Str, "illegal ascii value");
	    errorIn_Progress = 1;
	    return -2;
	}
    }

    return i-1; // we read one too far
}

// Returns: lenght of escape sequence if fine; -2 if not
// Upon entry, *last point to '\\'. 
// Upon exit, it points to the last valid character in the escape sequence
int
validEscape(int* Last, std::string& tmp_Str)
{
    int len = 1;

    *Last = getNext();
    switch(*Last){

    case 'a': case 'b': case 'f': case 'n': case 'r': case 't':
    case 'v': case '\\': case '\?': case '\'': case '\"':
	tmp_Str += *Last;
	break;

    case '0':
	tmp_Str += *Last;
	len += readOctHex(Last, tmp_Str, 8);
	if (errorIn_Progress) return -2;
	if ( (1 == len) ){ // \0[non-octal]
	    lexerError(0, tmp_Str, "illegal escape sequence");
	    errorIn_Progress = 1;
	    return -2;
	}
	break;

    case 'x': case 'X':
	tmp_Str += *Last; 
	len += readOctHex(Last, tmp_Str, 16);
	if (errorIn_Progress) return -2;
	if ( (1 == len) ){ // \x[non-octal]
	    lexerError(0, tmp_Str, "illegal escape sequence");
	    errorIn_Progress = 1;
	    return -2;
	}
	break;

    default: 
	std::string err_Str;
	err_Str += static_cast<char> (*Last);
	lexerError(0, err_Str, "");
	errorIn_Progress = 1;
	return -2;
	break;
    }

    return len;
}

// upon return, points to first digit (if any) of number
int
getBase(int* Last)
{
    if ( ('0' != (*Last)) )
	return 10;
    else{
	if ( ('x' == ((*Last) = getNext())) || ('X' == (*Last)) ){
	    *Last = getNext();
	    return 16;
	}
	else if ( isdigit(*Last) )
	    return 8;
	else{
	    putBack(*Last);
	    *Last = '0';
	    return 10;
	}
    }
}

// Returns: tok_int if octal/hex; 0 if so far correct processing of a dec int;
//          token(tok_err) if error
// Note:    returning tok_err could be a perfectly valid integer; hence,
//          caller is expted to check errorIn_Prog instead
// (handled differently as octal/hex can only be integers; but for decimals,
// we might have parsed only the 'x' part of x.y[e[+|-]z]] )
int
readIntValue(int* Last, int* pBase, int* Count, long* iV, std::string& tmp_Str){

    char* end_Ptr;
    int base = *pBase;

    if ( (8 == base) || (16 == base) ){
	do{
	    if ( (MAX_LIT == ++(*Count)) ){
		tooLongError(tmp_Str, "MAX_LIT", MAX_LIT);
		errorIn_Progress = 1;
		return tok_err;
	    }
	    tmp_Str += (*Last);
	    (*Last) = getNext();
	} while ( ((8==base) && isdigit(*Last)) || 
		  ((16==base) && isxdigit(*Last)) );

	if ( (16 == base) && (isalpha(*Last)) ) // catch 0x1ag
	    tmp_Str += (*Last);

	*iV = strtol(tmp_Str.c_str(), &end_Ptr, base);
	if ( (tmp_Str.c_str() == end_Ptr) || 
	     ('\0' != end_Ptr[0]) || ( 0 != errno)){
	    if (8 == base)
		tmp_Str.insert(0, "0");
	    else if (16 == base)
		tmp_Str.insert(0, "0x");
	    strToNumError(tmp_Str, "integer", end_Ptr[0]);
	    errorIn_Progress = 1;
	    return tok_err;
	}
	return tok_intV;
    }

    // dealing with decimal input
    do{
	if ( (MAX_LIT == ++(*Count)) ){
	    tooLongError(tmp_Str, "MAX_LIT", MAX_LIT);
	    errorIn_Progress = 1;
	    return tok_err;
	}
	tmp_Str += (*Last);
	(*Last) = getNext();
    } while (isdigit(*Last) );

    return 0;
}

// getTok() - return the next token from 'input'-stream
// invariant: upon return (other than from EOF), last_Char has the next
//            unprocessed char
token
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
	    if ( (MAX_ID == ++i) ){
		tooLongError(id_Str, "MAX_ID", MAX_ID);
		errorIn_Progress = 1;
		return token(tok_err);
	    }
	    id_Str += last_Char;
	}

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

	// calculate number as check for valid number/possible future use
	ret = readIntValue(&last_Char, &base, &i, &iV, id_Str);
	if (errorIn_Progress) return token(tok_err);

	if ( (tok_intV == ret) ){  // if we found an oct/hex int, we are done
	    tmp_Str << iV; // but return as string
	    id_Str = tmp_Str.str();
	    return token(tok_intV, id_Str);	
	}

	if ( ('.' == last_Char) ){ 
	    if ( (MAX_LIT == ++i) ){
		tooLongError(id_Str, "MAX_LIT", MAX_LIT);
		errorIn_Progress = 1;
		return token(tok_err);
	    }
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
		strToNumError(id_Str, "integer", end_Ptr[0]);
		errorIn_Progress = 1;
		return tok_err;
	    }
	    tmp_Str << iV;
	    id_Str = tmp_Str.str();
	    return token(tok_intV, id_Str);	
	}

	// integer after '.'
	while ( isdigit(last_Char) ){
	    if ( (MAX_LIT == ++i) ){
		tooLongError(id_Str, "MAX_LIT", MAX_LIT);
		errorIn_Progress = 1;
		return token(tok_err);
	    }
	    id_Str += last_Char;
	    getNext();
	}

	// deal with scientific notation
	if ( ('e' == last_Char) || ('E' == last_Char) ){
	    if ( (MAX_LIT == ++i) ){
		tooLongError(id_Str, "MAX_LIT", MAX_LIT);
		errorIn_Progress = 1;
		return token(tok_err);
	    }
	    id_Str += last_Char;
	    getNext();
	    id_Str += last_Char;
	    if ( !(std::isdigit(last_Char)) && 
		 ('-' != last_Char ) && ('+' != last_Char) ){
		lexerError(0, id_Str, "");
		errorIn_Progress = 1;
		return token(tok_err);
	    }
	    while ( isdigit(getNext())  ){
		if ( (MAX_LIT == ++i) ){
		    tooLongError(id_Str, "MAX_LIT", MAX_LIT);
		    errorIn_Progress = 1;
		    return token(tok_err);
		}
		id_Str += last_Char;
	   }
	}

	fV = strtod(id_Str.c_str(), &end_Ptr);
	if ((id_Str.c_str() == end_Ptr)||('\0' != end_Ptr[0])|| ( 0 != errno) ){
	    strToNumError(id_Str, "float", end_Ptr[0]);
	    errorIn_Progress = 1;
	    return token(tok_err);
	}
	tmp_Str << fV;
	id_Str = tmp_Str.str();
	return token(tok_doubleV, id_Str);
    } // end 'if number' scope

    // process strings
    if ( ('\"' == last_Char) ){	
	int i = 0;
	id_Str.clear();
	while ( ('\"' != getNext()) && (EOF != last_Char)){
	    if ( (MAX_STR == ++i) ){
		tooLongError(id_Str, "MAX_STR", MAX_STR);
		errorIn_Progress = 1;
		return token(tok_err);
	    }
	    else if ( ('\n' == last_Char) || (EOF == last_Char) ){
		lexerError(0, id_Str, "string missing closing \"");
		errorIn_Progress = 1;
		return token(tok_err);
	    }
	    else if ( ('\\' == last_Char) ){
		id_Str += '\\';
		i += validEscape(&last_Char, id_Str);
		if (errorIn_Progress) return token(tok_err);
		if ( (MAX_STR < i) ){
		    tooLongError(id_Str, "MAX_STR", MAX_STR);
		    errorIn_Progress = 1;
		    return token(tok_err);
		}
	    }
	    else
		id_Str += last_Char;
	}
	getNext();
	return token(tok_stringV, id_Str);
    }

/* done in preprocessor (keep for re-use later)
    // eat comments
    if ( ('/' == last_Char) ){
	if ('/' == getNext()){
	    while ( ('\n' != getNext()) && (EOF != last_Char) )
		;
	}
	else if ( ('*' == last_Char) ){ // comment type 2
	    id_Str = ""; // used to "beginning of comment type 2
	    for (;;){ // need infinite loop to allow (begin c) * (end c) type 
		id_Str += getNext();
		if ( (EOF == last_Char) ){
		    lexerError(0, id_Str, "comment missing closing (end c)");
		    errorIn_Progress = 1;
		    return token(tok_err);
		}
		else if ( ('*' == last_Char) ){
		    if ( ('/' == getNext()) ){
			id_Str.clear();
			break; // found a type 2 comment
		    }
		    else
			putBack(static_cast<char>(last_Char));
		}
	    }
	} // end loop for type 2 comments
	else{ // found a '/' char
	    if ( ('=' == last_Char) ){
		id_Str = "/=";
		getNext();
		return token(tok_assign_div);
	    }
	    else{
		id_Str = "/";
		return token(tok_div);
	    }
	}

	// If we come here, we are at in one of two situations:
	// Comment type 1: pointing to '\n' or EOF at proper comment line
	//         type 2: pointing at the closing tag (last 2 chars ).
	if ( last_Char != EOF){
	    getNext();
	    return getTok(); // re-throw when done with comment line
	}
	else 
	    return token(tok_eof);
    }
*/

    // did we hit EOF?
    if ( (EOF == last_Char) )
	return token(tok_eof);

    // 2 character operator tokens
    if ( ('/' == last_Char) ){ // re-located from old comment parsing
	if ( ('=' == getNext()) ){ 
	    getNext();
	    return token(tok_assign_div);
	}
	else
	    return token(tok_div);
    }
    if ( ('<' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_le);
	}
	else
	    return token(tok_lt);
    }
    if ( ('>' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_ge);
	}
	else
	    return token(tok_gt);
    }
    if ( ('=' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_log_eq);
	}
	else
	    return token(tok_eq);
    }
    if ( ('!' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_log_ne);
	}
	else
	    return token(tok_log_not);
    }
    if ( ('&' == last_Char) ){
	if ( ('&' == getNext()) ){
	    getNext();
	    return token(tok_log_and);
	}
	else{
	    lexerError(0, "&", "");
	    errorIn_Progress = 1;
	    return token(tok_err);
	}
    }
    if ( ('|' == last_Char) ){
	if ( ('|' == getNext()) ){
	    getNext();
	    return token(tok_log_or);
	}
	else{
	    lexerError(0, "|", "");
	    errorIn_Progress = 1;
	    return token(tok_err);
	}
    }
    if ( ('[' == last_Char) ){
	if ( (']' == getNext()) ){
	    getNext();
	    return token(tok_sqopenclosed);
	}
	else
	    return token(tok_sqopen);
    }
    if ( ('+' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_assign_plus);
	}
	else
	    return token(tok_plus);
    }
    if ( ('-' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_assign_minus);
	}
	else
	    return token(tok_minus);
    }
    if ( ('*' == last_Char) ){
	if ( ('=' == getNext()) ){
	    getNext();
	    return token(tok_assign_mult);
	}
	else
	    return token(tok_mult);
    }
    // "/=" handled during handling of comments

    // If we come here, we are down to single character operators and 
    // punctuation symbols, and illegal characters (sent as token(tok_err)).
    // Note that we also need to ready the next last_Char
    token tmpT = retOpPunct(last_Char);
    getNext();
    return tmpT;
}
