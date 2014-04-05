/********************************************************************
* ast.h - AST for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*
* Error checking: we assume parser hands on only arguments already
*                 checked for correctness
*
********************************************************************/

#ifndef AST_H_
#define AST_H_

#include <string>
#include <sstream>
#include "lexer.h"

int typePriority(std::string);
int typeWidth(std::string);

/***************************************
* Base classes
***************************************/

class NodeAST{
public:
    NodeAST(void)
	: line_(lineNo), col_(colNo) {}
    ~NodeAST() {}

    virtual void printLabel(void) { std::cout << "L" << label_Count_++ << ":"; }

private:
    int line_;
    int col_;
    static int label_Count_;
};

class ExprAST: public NodeAST{
public:
    // token* will actually be word*/intType*/fltType* objects
    // either object has been heap-allocated (in lexer.cpp)
    ExprAST(token* Type, token* WhichE)
	: NodeAST()
    {
	type_ = (dynamic_cast<word*>(Type))->Lexeme();
	delete Type;
	whichE_ = WhichE;
	typeW_ = typeWidth(type_);
	typeP_ = typePriority(type_);
    }
    ~ExprAST() { delete whichE_; }

    friend int typePriority(std::string);
    friend int typeWidth(std::string);

    int typeW(void) const { return typeW_; }
    int typeP(void) const { return typeP_; }

    virtual ExprAST& get(void) { return *this; } // object/rvalue
    virtual ExprAST& reduce(void) { return *this; } // address
    // TO ADD: jumping code

    virtual void toString(void) const
    {
	std::string tmp = (dynamic_cast<word*>(whichE_))->Lexeme();
	std::cout << tmp;
    }

private:
    std::string type_;
    int typeW_;
    int typeP_;
    token* whichE_;
};

/***************************************
* Expression terminals
***************************************/

class TempAST: public ExprAST{
public:
    TempAST(token* Type, token* WhichE)
	: ExprAST(Type, WhichE) { number_ = ++count_; }

    void toString(void) const
    {
	std::stringstream tmp;
	tmp << "t" << number_;
	std::cout << tmp;
    }

private:
    static int count_;
    int number_;
};

// ******************TO DO: methods gen(), reduce(), toSTring()

class IdAST: public ExprAST{
public:
    IdAST(token* Type, token* WhichE)
	: ExprAST(Type, WhichE) 
    { 
	name_ = (dynamic_cast<word*>(WhichE))->Lexeme();
    }

    std::string Name(void) const { return name_;}

private: // extract info for easier access
    std::string name_;
};

class intLitAST: public ExprAST{
public:
    intLitAST(token* Type, token* WhichE)
	: ExprAST(Type, WhichE)
    { 
	value_ = (dynamic_cast<intType*>(WhichE))->Value();
    }

    int Value(void) const { return value_;}

private: // extract info for easier access
    int value_;
};

class fltLitAST: public ExprAST{
public:
    fltLitAST(token* Type, token* WhichE)
	: ExprAST(Type, WhichE)
    { 
	value_ = (dynamic_cast<fltType*>(WhichE))->Value();
    }

    double Value(void) const { return value_;}

private: // extract info for easier access
    double value_;
};

class stringAST{
public:
    stringAST(token* WhichE)
    {
	std::string memString_ = (dynamic_cast<word*>(WhichE))->Lexeme();
    }

    std::string Value(void) const { return memString_;}

private:
    std::string memString_;  
};

#endif
