/********************************************************************
* parser.h - header file for parser.cpp 
*
********************************************************************/

#ifndef PARSER_H_
#define PARSER_H_

class Node_AST;
class Block_AST;
class Expr_AST;

extern Node_AST* pFirst_Node;

extern int frame_Depth;

Expr_AST* parseExpr(int Infix);
Block_AST* parseBlock(void);
int match(int, tokenType, int);

#endif
