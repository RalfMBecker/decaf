/********************************************************************
* parser.cpp - Parser for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*                (also known as Dijksta-Shunting algorithm) to flatten
*                the deep cfg
*
* Invariant: upon return, parse functions ensure to point ahead
*
* - will address mem leaks later - 
*
********************************************************************/

#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"

token next_Token;

token
getNextToken(void) { return (next_Token = getTok()); }

/***************************************
*  Primary expressions
***************************************/

int
match(int updatePrior, tokenType t, int updatePost)
{
    if (updatePrior) getNextToken();
    if ( (t == next_Token.Tok()) ){
	if (updatePost) getNextToken();
	return 0;
    }
    else
	return -1;
}

Expr_AST*
parseIntExpr(void)
{
    Expr_AST* res = new IntExpr_AST(next_Token);
    getNextToken();
    return res;
}

Expr_AST*
parseFltExpr(void)
{
    Expr_AST* res = new FltExpr_AST(next_Token);
    getNextToken();
    return res;
}

// ******NEEDS EXTENSION ONCE FUNCTIONS ALLOWED***********
// *******************************************************
// Used for *reading an ID*, not its definition
// Disambiguates shadowed names; returning the currently active one.
Expr_AST*
parseIdExpr(std::string Name)
{
    // TO DO: once we also catch arrays/fcts, might need change
    Expr_AST* Id;
    if ( (0 == (Id = findNameInHierarchy(top_Env, Name))) )
	throw(Primary_Error(Name, "not defined"));

    if ( (0 == match(1, tok_rdopen, 0)) )
	; // TO DO: this can catch function definitions
    else if ( (0 == match(0, tok_sqopen, 0)) )
	; // TO DO: this can catch arrays
    else
	return Id;
    return 0; // to suppress gcc warning
}

/*
Expr_AST*
parseParensExpr(void)
{
    getNextToken(); // create ready-state
    Expr_AST* E = parseExpr();

    if ( (-1 == match(0, tok_rdclosed, 1)) )
	throw(Punct_Error(')', 0));

    return E;
}
*/

// Primary -> id | intVal | fltVal | (expr) | -expr
Expr_AST*
parsePrimaryExpr(void)
{
    switch(next_Token.Tok()){
    case tok_intV: return parseIntExpr();
    case tok_doubleV: return parseFltExpr();
    case '(': //return parseParensExpr();
    case tok_ID: // note: coming here, ID has already been entered into
	         //       the symbol table
	return parseIdExpr(next_Token.Lex());

    default: throw(Primary_Error(next_Token.Lex(), "Expected Primary")); break;
    }
}

