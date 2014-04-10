/********************************************************************
* parser.h - header file for parser.cpp 
*
********************************************************************/

#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"

extern token next_Token;

ExprAST* ParseIntExpr(void);
ExprAST* ParseFltExpr(void);
ExprAST* ParseIdExpr(std::string name);
ExprAST* ParsePrimaryExpr(void);
ExprAST* ParseParensExpr(void);
ExprAST* ParseExpr(void);

#endif
