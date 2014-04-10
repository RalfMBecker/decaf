/******************************************************
* error.h -   basic error classes                     *
*                                                     *
******************************************************/

#ifndef ERROR_H_
#define ERROR_H_

#include <iostream>
#include <sstream>
#include <string>

#include <cstdlib>     // exit(), etc.
#include <cstdio>      // perror()
#include <errno.h>     // errno()

#include "lexer.h"
#include "driver.h"
#include "ast.h"

const int MAX_TEXT = 32;

extern int no_lex_Errors;
extern int no_par_Errors;

void errExit(int pError, const char* msg, ...);

class Error{
public:
Error(int Line = lineNo, int Col = colNo)
    : line_(Line), col_(Col) {}
    ~Error() {}

    virtual void print() const 
    { std::cerr << "Error near " <<  line_ << ":" << col_ << ": "; };
private:
    int line_;
    int col_;
};

class Primary_Error: public Error{
public: // in current setting, have no AST object to throw
Primary_Error(std::string token_Str, std::string comment_Str) 
    : Error(), str1_(token_Str), str2_(comment_Str)
    {
	no_par_Errors++;
    }

    virtual void print() const
    {
	Error::print();
	std::ostringstream tmp_Str;
	tmp_Str << "Syntax error (" << str1_ << ") - " << str2_;
	std::cerr << tmp_Str;
    }

private:
    std::string str1_;
    std::string str2_;
};

class Punct_Error: public Error{
public:
Punct_Error(char c, int What)
    : punct_(c), what_(What)
    {
	if ( (0!= what_) && (1!= what_) )
	    errExit(0, "invalid arg of Punct_Error (%d)\n", what_); 
	no_par_Errors++;
    }

    virtual void print() const
    {
	Error::print();
	std::ostringstream tmp_Str;
	tmp_Str << "Syntax error - ";
	if ( (0 == what_) ) tmp_Str << "missing ";
	else tmp_Str << "stray ";
	tmp_Str << "token (" << punct_ << ")\n";
	std::cerr << tmp_Str;
    }

private:
    char punct_;
    int what_; // 0: missing; 1: spare
};

// *********** update when better understood how used ********
// (eg, ternary private variables: lhs op rhs?
// basic lexer error class
class Lexer_Error: public Error{
public:
Lexer_Error(const std::string& name, const std::string& second)
    : Error(), Name(name), Second(second)
    {
	no_lex_Errors++;
    }

    virtual void print() const; // we rely on coercion of a reference, so must
                        // make this function const
private: 
    std::string Name; 
    std::string Second;
};

// Enforcing max lengths of identifiers, etc. Sample usage:
//            name=<identifier>, type_Str="MAX_ID", type=MAX_ID
class tooLongError: public Lexer_Error{
public:
tooLongError(const std::string& name, const std::string& type_Str, int type)
    : Lexer_Error(name, ""), typeName(type_Str), typeValue(type) {}

    void print() const
    {
	Lexer_Error::print();
	std::cerr << "longer than " << typeName << " (" << typeValue << ")\n";
    }
private:
    std::string typeName;
    int typeValue;
};

// strtod has arcane error repoting; c./ man page
class strtoNumError: public Lexer_Error{
public:
strtoNumError(const std::string& name,const std::string& type, char c)
    : Lexer_Error(name, ""), typeName(type), offending(c) {}

    void print() const{
	Lexer_Error::print();
	if ( 0 != errno){ // overflow/underflow
	    std::cerr << typeName << " - ";
	    if ( ("float" == typeName) )
		perror("strtod");
	    else if ( ("integer" == typeName) )
		perror("strtol");
	    else{
		std::cerr << "invalid use of function strtoNumError\n";
		exit(EXIT_FAILURE);
	    }
	}
	else if ( ('\0' != offending) )
	    std::cerr << " - offending character: " << offending << "\n";
    }
private:
    std::string typeName;
    char offending;
};

// If called for a Lexer_Error object with Second == "", we want to print
// a '\n'; however, if it is called from a derived class, we want to keep
// printing errors from that class, and suppress the '\n'.
//
// The syntax chosen (a const_cast followed by a dynamic_cast) is
// probably not ideal, but we need the function to be const (see class
// declaration above).
//
// If we don't define this inline after tooLongError, we have an incomplete
// type reference
inline void Lexer_Error::print() const
{
    Lexer_Error* pThis = const_cast<Lexer_Error*>(this);
    std::ostringstream tmp_Str;

    Error::print();
    tmp_Str << "token (" << Name << ")";
    if ( Second.empty() ){
	if ( !(dynamic_cast<tooLongError*>(pThis)) &&
	     !(dynamic_cast<strtoNumError*>(pThis)) )
	    tmp_Str << "\n";
	else
	    tmp_Str << " ";
    }
    else
	tmp_Str << " - " << Second << "\n";
    std::cerr << tmp_Str.str();
}

#endif
