/***********************************************************************
* error.h -   basic error classes                     
*                                                     
* Stroustrup: "(throw-catch) Exception handling is intended for dealing 
*             with non-local problems." (Special Edition, p. 193)
*
* Philosophy: We started out using an Error Class hierarchy, and
*             try-catch blocks in the driver. This does not work (well)
*             for incremental error REPORTS followed by LOCAL error
*             recovery (we keep compiling to catch as many errors in one
*             compile iteration as possible); and local try-catch semantic
*             in the parser would make the code a mess.
* Solution:   use error functions, not classes
* 
* Why not classes and their members, used locally by caller? :
*             Calling member classes of an error class hierarchy from 
*             the parser, say, leads to tricky cross-dependency issues
*             and code that is hard, if not impossible, to compile. 
*
************************************************************************/

#ifndef ERROR_H_
#define ERROR_H_

#include <iostream>
#include <sstream>
#include <string>

#include <cstdlib>     // exit(), strtol(), etc.
#include <cstdio>      // perror()
#include <errno.h>     // errno()

#include "lexer.h"
#include "ast.h"
#include "driver.h"

const int MAX_TEXT = 32;

extern int no_lex_Errors;
extern int no_par_Errors;

void errExit(int pError, const char* format, ...);

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

class Semantic_Error: public Error{
public:
    ~Semantic_Error() {}
};

// Type: 0 - not defined; 1 - attempt to redefine
class VarAccess_Error: public Semantic_Error{
public:
VarAccess_Error(std::string Name, int Type)
    : Semantic_Error(), name_(Name), type_(Type)
    {
	no_par_Errors++;
    }

    virtual void print() const
    {
	Error::print();
	std::ostringstream tmp_Stream;
	tmp_Stream << "attempt to ";
	if ( (0 == type_) )
	    tmp_Stream << "access undefined ";
	else if ( (1 == type_) )
	    tmp_Stream << "redefine already defined ";
	else
	    errExit(0, "illegal use of VarAccess_Error");
	tmp_Stream << "variable (" << name_ << ")\n";

	std::cerr << tmp_Stream.str();
    }

private:
    std::string name_;
	int type_;
};

class Primary_Error: public Semantic_Error{
public: // in current setting, have no AST object to throw
Primary_Error(std::string token_Str, std::string comment_Str) 
    : Semantic_Error(), str1_(token_Str), str2_(comment_Str)
    {
	no_par_Errors++;
    }

    virtual void print() const
    {
	Error::print();
	std::ostringstream tmp_Str;
	tmp_Str << "Syntax error (" << str1_ << ") - " << str2_ << "\n";
	std::cerr << tmp_Str.str();
    }

private:
    std::string str1_;
    std::string str2_;
};

// What - 0: missing; 1: spare
class Punct_Error: public Semantic_Error{
public:
Punct_Error(char c, int What)
    : Semantic_Error(), punct_(c), what_(What)
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
	std::cerr << tmp_Str.str();
    }

private:
    char punct_;
    int what_; // 0: missing; 1: spare
};

// basic lexer error class
class Lexer_Error: public Error{
public:
Lexer_Error(const std::string& Name, const std::string& Second)
    : Error(), name_(Name), second_(Second)
    {
	no_lex_Errors++;
    }

    virtual void print() const; // we rely on coercion of a reference, so must
                        // make this function const
private: 
    std::string name_; 
    std::string second_;
};

// Enforcing max lengths of identifiers, etc. Sample usage:
//            name=<identifier>, type_Str="MAX_ID", type=MAX_ID
class TooLong_Error: public Lexer_Error{
public:
TooLong_Error(const std::string& Name, const std::string& type_Str, int Type)
    : Lexer_Error(Name, ""), typeName_(type_Str), typeValue_(Type) {}

    void print() const
    {
	Lexer_Error::print();
	std::cerr << "longer than " << typeName_ << " (" << typeValue_ << ")\n";
    }
private:
    std::string typeName_;
    int typeValue_;
};

// strtod has arcane error repoting; c./ man page
class StrToNum_Error: public Lexer_Error{
public:
StrToNum_Error(const std::string& Name,const std::string& Type, char c)
    : Lexer_Error(Name, ""), typeName_(Type), offending_(c) {}

    void print() const{
	Lexer_Error::print();
	if ( 0 != errno){ // overflow/underflow
	    std::cerr << typeName_ << " - ";
	    if ( ("float" == typeName_) )
		perror("strtod");
	    else if ( ("integer" == typeName_) )
		perror("strtol");
	    else
		errExit(0, "invalid use of function StrToNum_Error\n");
	}
	else if ( ('\0' != offending_) )
	    std::cerr << " - offending character: " << offending_ << "\n";
    }
private:
    std::string typeName_;
    char offending_;
};

// If called for a Lexer_Error object with Second == "", we want to print
// a '\n'; however, if it is called from a derived class, we want to keep
// printing errors from that class, and suppress the '\n'.
//
// The syntax chosen (a const_cast followed by a dynamic_cast) is
// probably not ideal, but we need the function to be const (see class
// declaration above).
//
// If we don't define this inline after TooLong_Error, we have an incomplete
// type reference
inline void Lexer_Error::print() const
{
    Lexer_Error* pThis = const_cast<Lexer_Error*>(this);
    std::ostringstream tmp_Str;

    Error::print();
    tmp_Str << "token (" << name_ << ")";
    if ( second_.empty() ){
	if ( !(dynamic_cast<TooLong_Error*>(pThis)) &&
	     !(dynamic_cast<StrToNum_Error*>(pThis)) )
	    tmp_Str << "\n";
	else
	    tmp_Str << " ";
    }
    else
	tmp_Str << " - " << second_ << "\n";
    std::cerr << tmp_Str.str();
}

#endif
