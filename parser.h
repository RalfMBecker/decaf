/********************************************************************
* parser.h - header file for parser.cpp 
*
********************************************************************/

#ifndef PARSER_H_
#define PARSER_H_

class Node_AST;
class Expr_AST;

extern Node_AST* pFirst_Node;

extern token next_Token;
token getNextToken(void);

Expr_AST* parseExpr(int Infix);
Block_AST* parseBlock(void);

#endif
