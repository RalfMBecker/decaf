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
#include "lexer.h"

/***************************************
* Base classes
***************************************/

class Node{
public:
    Node(void)
	: line(lineNo), col(colNo) {}
    ~Node() {}

    virtual void printLabel(void) { std::cout << "L" << label_Count++ << ":"; }

protected:
    int line;
    int col;
private:
    static int label_Count;
};

class Expression: public Node{
public:
    // token* will actually be a word*/intType*/fltType* object
    // either object has been heap-allocated (in lexer.cpp)
    Expression(token* Type, token* WhichE)
	: Node()
    {
	type = Type;
	whichE = WhichE;
    }
    ~Expression()
    {
	delete type;
	delete whichE;
    }



private:
    token* type;
    token* whichE;
};


/***************************************
* Expression terminals
***************************************/


/***************************************
* Arithmetic expression classes
***************************************/


/*
// *********************************
// Terminals
// *********************************
class opAST{
public:
    opAST(tokenType which, std::string which_Str)
	: type(which), rep(which_Str) {}

private:
    tokenType type;
    std::string rep;
};

class idAST{
public:
    idAST(std::string nameStr)
	: name(nameStr) {}

private:
    std::string name;
};

class intLitAST{
public:
    intLitAST(tokenType which, long Value)
	: type(which), value(Value) {}

private:
    tokenType type;
    long value;
};

class floatLitAST{
public:
    floatLitAST(tokenType which, double Value)
	: type(which), value(Value) {}

private:
    tokenType type;
    double value;
};


// *********************************
// Expressions
// *********************************
class expressionAST{
public:
    ~expressionAST();
};

// ************** HOW TO STORE PRIVATE VARS AND HOW TO HAND ON ARGS********
class infixExpressionAST: public expressionAST{
public:
    infixExpressionAST(opAST Op, expressionAST El, expressionAST Er)
	: op(Op), LHS(El), RHS(Er) {}

private:
    opAST op;
    expressionAST LHS;
    expressionAST RHS;
};
*/


#endif
