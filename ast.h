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
	: line_(lineNo), col_(colNo) {}
    ~Node() {}

    virtual void printLabel(void) { std::cout << "L" << label_Count++ << ":"; }

protected:
    int line_;
    int col_;
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
	type_ = Type;
	whichE_ = WhichE;
    }
    ~Expression()
    {
	delete type_;
	delete whichE_;
    }



private:
    token* type_;
    token* whichE_;
};

/***************************************
* Expression terminals
***************************************/


#endif
