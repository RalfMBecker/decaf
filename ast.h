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



#endif
