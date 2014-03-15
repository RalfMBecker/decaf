/******************************************************
* error.cpp -   basic error classes                   *
*                                                     *
******************************************************/

#ifndef ERROR_H_
#define ERROR_H_

#include <iostream>
#include <sstream>
#include <string>
#include "lexer.h"

class Error{
public:
    virtual void print() const 
    { std::cerr << "Error in line " <<  lineNo << ": "; };
};

// *********** update when better understood how used ********
// (eg, ternary private variables: lhs op rhs?
class ExprAST_Error: public Error{
public:
ExprAST_Error(const std::string& what)
    : What(what) {}
    void print() const { 
	Error::print();
	std::cerr << "in expression (" << What << ")\n";
    }
private: 
    std::string What;
};

// so far only print out prototype name
class PrototypeAST_Error: public Error{
public:
PrototypeAST_Error(const std::string& name)
    : Name(name) {}
    void print() const { 
	Error::print();
	std::cerr << "in Prototype (" << Name << ")\n"; 
    }
private: 
    std::string Name;
};

// again, only print out function name (which is of "x.y.name" form)
class FunctionAST_Error: public Error{
public:
FunctionAST_Error(const std::string& name)
    : Name(name) {}
    void print() const { 
	Error::print();
	std::cerr << "in expression (" << Name << ")\n"; 
    }
private: 
    std::string Name;
};

// basic lexer error class
class Lexer_Error: public Error{
public:
Lexer_Error(const std::string& name, const std::string& second)
    : Name(name), Second(second) {}
    void print() const {
	std::ostringstream tmp_Str;
	Error::print();
	tmp_Str << "illegal token (" << Name << ")";
	if ( Second.empty() )
	    tmp_Str << "\n";
	else
	    tmp_Str << ": " << Second << "\n";
	std::cerr << tmp_Str.str();
    }
private: 
    std::string Name; 
    std::string Second;
};

#endif
