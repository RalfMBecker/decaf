/********************************************************************
* parser.cpp - Parser for Decaf
*
* Notes: Operator Precedence parsing for expressions to flatten the
*        the CFG
*
* Invariant: upon return, parse functions ensure to point ahead
*
* - will address mem leaks later - 
*
********************************************************************/

#include "ast.h"
#include "lexer.h"
#include "error.h"
#include "tables.h"

token next_Token;

/*
int typeWidth(std::string const&);
int typePriority(std::string);
ExprAST* findNameInHierarchy(Env*, std::string);
*/
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
// *******************************************************
// Used for *reading an ID*, not its definition
// Disambiguates shadowed names; returning the currently active one.
ExprAST*
ParseIdExpr(std::string Name)
{
    // TO DO: once we also catch arrays/fcts, might need change
    ExprAST* Id;
    if ( (0 == (Id = findNameInHierarchy(top_Env, Name))) )
	throw(Primary_Error(Name, "not defined"));

    if ( (0 == match(1, tok_rdopen, 0)) )
	; // TO DO: this can catch function definitions
    else if ( (0 == match(0, tok_sqopen, 0)) )
	; // TO DO: this can catch arrays
    return 0;
}

/*
ExprAST*
ParseParensExpr(void)
{
    getNextToken(); // create ready-state
    ExprAST* E = ParseExpr();

    if ( (-1 == match(0, tok_rdclosed, 1)) )
	throw(Punct_Error(')', 0));

    return E;
}
*/

// Primary -> id | intVal | fltVal | (expr) | -expr
ExprAST*
ParsePrimaryExpr(void)
{
    switch(next_Token.Tok()){
    case tok_intV: return ParseIntExpr();
    case tok_doubleV: return ParseFltExpr();
    case '(': //return ParseParensExpr();
    case tok_ID: // note: coming here, ID has already been entered into
	         //       the symbol table
	return ParseIdExpr(next_Token.Lex());

    default: throw(Primary_Error(next_Token.Lex(), "Expected Primary")); break;
    }
}

