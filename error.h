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
#include <cstring>

#define MAX_TEXT 32

class Error{
public:
    virtual void print() const 
    { std::cerr << "Error near " <<  lineNo << ":" << colNo << ": "; };
};

// *********** update when better understood how used ********
// (eg, ternary private variables: lhs op rhs?
class ExprAST_Error: public Error{
public:
ExprAST_Error(const std::string& what)
    : What(what) {}
    void print() const
    { 
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
    void print() const
    {
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
    void print() const
    { 
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
    strtoNumError(const std::string name,const std::string type,char* end_Ptr)
	: Lexer_Error(name, ""), typeName(type)
    {	
	endPtr = new char[MAX_TEXT];
	strcpy(endPtr, end_Ptr);
    }

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
	else if ( (0 != endPtr) )
	    std::cerr << " - offending character: " << *endPtr << "\n";
    }
private:
    std::string typeName;
    char* endPtr;
};

// if this is called for a Lexer_Error object with Second == "", we 
// want to print a '\n'; however, if it is called from a derived class,
// we want to keep printing errors from that class, and suppress the '\n'
// the syntax chosen to do this (const case followed by dynamic_cast is
// probably not ideal, but we need the function to be const (see class
// declaration above)
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
