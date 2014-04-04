/********************************************************************
* lexer.h - header file for lexer for Decaf
*
********************************************************************/

#ifndef LEXER_H_
#define LEXER_H_

#include <iostream>
#include <string>

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
    tok_semi = ';', tok_comma = ',', tok_sqclosed = ']', tok_rdopen = '(',
    tok_rdclosed = ')', tok_paropen = '{', tok_parclosed = '}',
    // AST types
    tok_ID = -100, tok_tmp = -101, 
};

class token{
public:
    token(tokenType t = tok_eof)
	: name_(t) {}
    ~token() {}

    virtual tokenType Name(void) const { return name_; } // provide 
                         // encapsulated access to derived classes

private:
    tokenType name_;
};

// for all but tok_ID, hand on lex=""
class word: public token{
public:
    word(tokenType t = tok_eof, std::string lex = "")
	: token(t)
    {
	switch(t){ 
	case tok_void: lexeme_ = "void"; break;
	case tok_int: lexeme_ = "int"; break;
	case tok_double: lexeme_ = "double"; break;
	case tok_bool: lexeme_ = "bool"; break;
	case tok_string: lexeme_ = "string"; break;

	case tok_return: lexeme_ = "return"; break;
	case tok_true: lexeme_ = "true"; break;
	case tok_false: lexeme_ = "false"; break;
	case tok_null: lexeme_ = "null"; break;
	case tok_for: lexeme_ = "for"; break;
	case tok_while: lexeme_ = "while"; break;
	case tok_if: lexeme_ = "if"; break;
	case tok_else: lexeme_ = "else"; break;
	case tok_break: lexeme_ = "break"; break;

	case tok_class: lexeme_ = "class"; break;
	case tok_interface: lexeme_ = "interface"; break;
	case tok_this: lexeme_ = "this"; break;
	case tok_extends: lexeme_ = "extends"; break;
	case tok_implements: lexeme_ = "implements"; break;

	case tok_new: lexeme_ = "new"; break;
	case tok_NewArray: lexeme_ = "newArray"; break;

	case tok_Print: lexeme_ = "Print"; break;
	case tok_ReadInteger: lexeme_ = "ReadInteger"; break;
	case tok_ReadLine: lexeme_ = "ReadLine"; break;

	case tok_le: lexeme_ = "<="; break;
	case tok_ge: lexeme_ = ">="; break;
	case tok_log_eq: lexeme_ = "=="; break;
	case tok_log_ne: lexeme_ = "!="; break;
	case tok_log_and: lexeme_ = "&&"; break;
	case tok_log_or: lexeme_ = "||"; break;
	case tok_sqopenclosed: lexeme_ = "[]"; break;

	case tok_tmp: lexeme_ = "t"; break;

	case tok_ID: 
	case tok_memString: lexeme_ = lex; break;

	    // ideally, all that follows would be a base class object
	    // preserving the polymorphism at this point, however, this 
	    // would be more work than it's worth.
	    // 1-char semantic ops
	case tok_plus: case tok_minus: case tok_mult: case tok_div:
	case tok_mod: case tok_lt: case tok_gt: case tok_eq:
	case tok_log_not: case tok_dot: case tok_sqopen:
	    // 1-char others
	case tok_semi: case tok_comma: case tok_sqclosed: case tok_rdopen:
	case tok_rdclosed: case tok_paropen: case tok_parclosed:
	    lexeme_ = std::string(1, static_cast<char>(t));
	    break;

	default:
	    break;
	}
    }

    std::string Lexeme_(void) const { return lexeme_; }

private:
    std::string lexeme_;
};

class intType: public token{
public:
    intType(tokenType t, long v)
	: token(t), value_(v) {}

    long Value(void) const { return value_; }

private:
    long value_;
}; 

class fltType: public token{
public:
    fltType(tokenType t, double v)
	: token(t), value_(v) {}

    double Value(void) const { return value_; }

private:
    double value_;
}; 

// it is more logical to make them extern here: they are defined in 
// lexer.cpp translation unit
extern std::string id_Str;  // for tok_identifier
extern long val_Int;       // for tok_number
extern double val_Flt; 

extern int lineNo;
extern int colNo;
extern int last_Char; // not ideal, but nice to be able to access elsewhere

token* getTok(void);
int getNext(void);

#endif
