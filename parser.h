/********************************************************************
* parser.h - header file for parser.cpp 
*
********************************************************************/

#ifndef PARSER_H_
#define PARSER_H_

class Expr_AST;

extern token next_Token;
token getNextToken(void);

Expr_AST* parseIntExpr(void);
Expr_AST* parseFltExpr(void);
Expr_AST* parseIdExpr(std::string name);
Expr_AST* parsePrimaryExpr(void);
Expr_AST* parseParensExpr(void);
Expr_AST* parseExpr(void);
Expr_AST* parseBlock(void);

#endif
