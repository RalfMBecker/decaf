/********************************************************************
* parser.h - header file for parser.cpp 
*
********************************************************************/

#ifndef PARSER_H_
#define PARSER_H_

class Expr_AST;

extern token next_Token;

Expr_AST* ParseIntExpr(void);
Expr_AST* ParseFltExpr(void);
Expr_AST* ParseIdExpr(std::string name);
Expr_AST* ParsePrimaryExpr(void);
Expr_AST* ParseParensExpr(void);
Expr_AST* ParseExpr(void);

#endif
