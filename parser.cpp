/********************************************************************
* tables.cpp - Tables for Decaf
*
* Notes: Operator Precedence parsing for expressions to flatten the
*        the CFG
*
********************************************************************/

#include "ast.h"
#include "lexer.h"
#include "error.h"

token next_Token;
token
getNextToken(void) { return (next_Token = getTok()); }

/***************************************
*  Primary expressions
***************************************/

ExprAST*
ParseIntExpr(void)
{
    ExprAST* res = new IntExprAST(next_Token);
    getNextToken();
    return res;
}

ExprAST*
ParseFltExpr(void)
{
    ExprAST* res = new FltExprAST(next_Token);
    getNextToken();
    return res;
}

// ******NEEDS EXTENSION ONCE FUNCTIONS ALLOWED***********
ExprAST*
ParseIdExpr(token Type)
{
    ExprAST* res = new IdExprAST(Type, next_Token);
    getNextToken();
    return res;
}

ExprAST*
ParsePrimaryExpr(void)
{
    switch(next_Token.Tok()){
    case tok_intV: return ParseIntExpr();
    case tok_doubleV: return ParseFltExpr();

    default: throw(Primary_Error(next_Token.Lex())); break;
    }
}

