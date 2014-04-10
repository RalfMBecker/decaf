/********************************************************************
* parser.cpp - Parser for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*                (also known as Dijksta-Shunting algorithm) to flatten
*                the deep cfg
*
* Invariant: upon return, parse functions ensure to point ahead
*
* Mem Leaks: will add Visitor to delete AST*s and Env* later
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

Expr_AST*
parseBoolExpr(void)
{
    Expr_AST* res = new BoolExpr_AST(next_Token);
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

Expr_AST* parsePrimaryExpr(void);
Expr_AST* parseInfixRHS(int, Expr_AST*);

Expr_AST*
parseExpr(void)
{
    Expr_AST* LHS = parsePrimaryExpr();
    if ( !LHS )
	throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));

    return parseInfixRHS(0, LHS); // don't impose any precedence on LHS
}

extern std::map<tokenType, int> bin_OpTable;

// Dijkstra shunting algorithm
// Example used in comments below: LHS + b * c
//         prec_1: precedence of LHS
//         prec_2: precedence of '+'
//         prec_3: precedence of '*'
Expr_AST*
parseInfixRHS(int prec_1, Expr_AST* LHS)
{ 
    std::string const errMsg = "Expected primary expression";
    for (;;){
	int prec_2 = opPriority(next_Token.Tok());
	if ( (prec_2 < prec_1) )
	    return LHS;
	token binOp1 = next_Token; // store it as we for later op creation

	getNextToken();
	Expr_AST* RHS = parsePrimaryExpr();
	if (!RHS)
	    throw(Primary_Error(next_Token.Lex(), errMsg));

	int prec_3 = opPriority(next_Token.Tok());
	if (prec_2 < prec_3){ // flip from l-r, to r-l (at least for one step)
	    RHS = parseInfixRHS(prec_2 + 1, RHS); // keep going l-r until 
	    if (!RHS)
		throw(Primary_Error(next_Token.Lex(), errMsg));
	}
// TO DO: coercion; NOT ONLY ARITHM; bin var to track # of relational
//        careful!
	LHS = new ArithmExpr_AST(binOp1, LHS, RHS);
    }
}

Expr_AST*
parseParensExpr(void)
{
    getNextToken(); // create ready-state
    Expr_AST* E = parseExpr();

    if ( (-1 == match(0, tok_rdclosed, 1)) )
	throw(Punct_Error(')', 0));

    return E;
}

// Primary -> id | intVal | fltVal | (expr) | -expr
Expr_AST*
parsePrimaryExpr(void)
{
    switch(next_Token.Tok()){
    case tok_intV: return parseIntExpr();
    case tok_doubleV: return parseFltExpr();
    case tok_true:
    case tok_false: return parseBoolExpr();
    case '(': return parseParensExpr();
    case tok_ID: // note: coming here, ID has already been entered into
	         //       the symbol table
	return parseIdExpr(next_Token.Lex());

    default: 
	throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));
	break;
    }
}

