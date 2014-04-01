/********************************************************************
* lexer.h - header file for lexer for Decaf
*
********************************************************************/

#ifndef LEXER_H_
#define LEXER_H_

#include <iostream>
#include <string>

// REVISIT 'class' FOR EOF AS NEEDED
enum tokenType{
    // misc
    tok_eof = -1, tok_return = -2, tok_memString = -3,
    // types
    tok_void = -10, tok_int = -11, tok_double = -12, tok_bool = -13,
    tok_string = -16,
    // type values
    tok_true = -20, tok_false = -21, tok_null = -22,
    // looping
    tok_for = -30, tok_while = -31, tok_if = -32, tok_else = -33, 
    tok_break = -34,
    // i/o
    tok_Print = -40, tok_ReadInteger = -41, tok_ReadLine = -42,
    // classes
    tok_class = -50, tok_interface = -51, tok_this = -52, tok_extends = -53,
    tok_implements = -54,
    // allocation/declaration
    tok_new = -60, tok_NewArray = -61,
    // 2 character operators
    tok_le = -70, tok_ge = -71, tok_log_eq = -72, tok_log_ne = -73, 
    tok_log_and = -74, tok_log_or = -75, tok_sqopenclosed = -76,
    // 1 character semantic operators
    tok_plus = '+', tok_minus = '-', tok_mult = '*', tok_div = '/',
    tok_mod = '%', tok_lt = '<', tok_gt = '>', tok_eq = '=',
    tok_log_not = '!', tok_sqopen = '[', tok_dot = '.',
    // 1 character non-semantic operators
    tok_semi = ';', tok_comma = ',', tok_sqclosed = '}', tok_rdopen = '(',
    tok_rdclosed = ')', tok_paropen = '{', tok_parclosed = '}',
    // AST types
    tok_ID = -100, tok_Int = -101, tok_flt = -102, // tok_expr = -103,
};

class token{
public:
    token(tokenType t)
	: name(t) {}

    tokenType Name(void) const { return name; }
    void print(void) const // used for, e.g., '+' 
    { 
	std::cout << static_cast<char>(name); 
    }
private:
    tokenType name;
};

// for all but tok_ID, hand on lex=""
class word: public token{
public:
    word(tokenType t, std::string lex)
	: token(t)
    {
	switch(t){ 
	case tok_void: lexeme = "void"; break;
	case tok_int: lexeme = "int"; break;
	case tok_double: lexeme = "double"; break;
	case tok_bool: lexeme = "bool"; break;
	case tok_string: lexeme = "string"; break;

	case tok_return: lexeme = "return"; break;
	case tok_true: lexeme = "true"; break;
	case tok_false: lexeme = "false"; break;
	case tok_null: lexeme = "null"; break;
	case tok_for: lexeme = "for"; break;
	case tok_while: lexeme = "while"; break;
	case tok_if: lexeme = "if"; break;
	case tok_else: lexeme = "else"; break;
	case tok_break: lexeme = "break"; break;

	case tok_class: lexeme = "class"; break;
	case tok_interface: lexeme = "interface"; break;
	case tok_this: lexeme = "this"; break;
	case tok_extends: lexeme = "extends"; break;
	case tok_implements: lexeme = "implements"; break;

	case tok_new: lexeme = "new"; break;
	case tok_NewArray: lexeme = "newArray"; break;

	case tok_Print: lexeme = "Print"; break;
	case tok_ReadInteger: lexeme = "ReadInteger"; break;
	case tok_ReadLine: lexeme = "ReadLine"; break;

	case tok_le: lexeme = "<="; break;
	case tok_ge: lexeme = ">="; break;
	case tok_log_eq: lexeme = "=="; break;
	case tok_log_ne: lexeme = "!="; break;
	case tok_log_and: lexeme = "&&"; break;
	case tok_log_or: lexeme = "||"; break;
	case tok_sqopenclosed: lexeme = "[]"; break;

	case tok_ID: 
	case tok_memString: lexeme = lex; break;

	default:
	    break;
	}
    }

    std::string Lexeme(void) const { return lexeme; }
    void print(void) const { std::cout << lexeme; } 

private:
    std::string lexeme;
};

class intType: public token{
public:
    intType(tokenType t, long v)
	: token(t), value(v) {}

    long Value(void) const { return value; }
    void print(void) const { std::cout << value; }

private:
    long value;
}; 

class fltType: public token{
public:
    fltType(tokenType t, double v)
	: token(t), value(v) {}

    double Value(void) const { return value; }
    void print(void) const { std::cout << value; }

private:
    double value;
}; 

// it is more logical to make them extern here: they are defined in 
// lexer.cpp translation unit
extern std::string id_Str;  // for tok_identifier
extern long val_Int;       // for tok_number
extern double val_Flt; 

extern int lineNo;
extern int colNo;
extern int last_Char; // not ideal, but nice to be able to access elsewhere

token getTok(void);
int getNext(void);

#endif
