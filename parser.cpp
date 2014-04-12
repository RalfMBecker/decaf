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
    std::cout << "matching...\n";
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
    std::cout << "parsing an int...\n";
    Expr_AST* res = new IntExpr_AST(next_Token);
    getNextToken();
    return res;
}

Expr_AST*
parseFltExpr(void)
{
    std::cout << "parsing a flt...\n";
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
    std::cout << "parsing an Id...\n";
    // TO DO: once we also catch arrays/fcts, might need change
    Expr_AST* Id;
    if ( (0 == (Id = findNameInHierarchy(top_Env, Name))) )
	throw(Primary_Error(Name, "not declared"));

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
parseExpr(int Infix) // 0 - no; 1 = yes [probably add 2 = logical prefix)
{
    std::cout << "parsing an expr...\n";
    Expr_AST* LHS = parsePrimaryExpr();
    if ( !LHS )
	throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));

    Expr_AST* ptmp_AST = 0;
    if (Infix) // TO DO: adjust for logical
	ptmp_AST = new UnaryArithmExpr_AST(token(tok_minus), LHS);
    if ( (0 == ptmp_AST) )
	return parseInfixRHS(0, LHS); // don't impose any precedence on LHS
    else
	return parseInfixRHS(0, ptmp_AST);
    return 0; // to suppress gcc warning

}

extern std::map<tokenType, int> bin_OpTable;

// Dijkstra shunting algorithm
// Example used in comments below: LHS + b * c - d
//         First time through:            After recursing when prec_2 < prec_3
//         prec_1: precedence of LHS      prec_2 + 1
//         prec_2: precedence of '+'      precedence of '*' (changed role)
//         prec_3: precedence of '*'      precedence of '-'
// The way we track recursion, could go bad for deeply recursive infix.
// Should be added to a full description of the compiler/language.
Expr_AST*
parseInfixRHS(int prec_1, Expr_AST* LHS)
{ 
    std::cout << "entering parseInfixRHS...\n";
    static int logOp_Tot = 0;
    std::string const err_Msg = "Expected primary expression";
    std::string const err_Msg2 = "illegal chaining of logical operators";
    for (;;){
	int prec_2 = opPriority(next_Token.Tok());
	std::cout << "current token (1) = " << next_Token.Lex() << "\n";
	logOp_Tot += isLogicalAdd(next_Token.Tok());
	if ( (1 < logOp_Tot) )
	    throw(Primary_Error(next_Token.Lex(), err_Msg2));

	if ( (prec_2 < prec_1) )
	    return LHS;
	token binOp1 = next_Token; // store it for later op creation

	getNextToken();
	Expr_AST* RHS = parsePrimaryExpr();
	if (!RHS)
	    throw(Primary_Error(next_Token.Lex(), err_Msg));

	int prec_3 = opPriority(next_Token.Tok());
	std::cout << "current token (2) = " << next_Token.Lex() << "\n";
	logOp_Tot += isLogicalAdd(next_Token.Tok());
	if ( (1 < logOp_Tot) )
	    throw(Primary_Error(next_Token.Lex(), err_Msg2));

	if (prec_2 < prec_3){ // flip from l-r, to r-l (at least for one step)
	    RHS = parseInfixRHS(prec_2 + 1, RHS); // keep going l-r until 
	    if (!RHS)
		throw(Primary_Error(next_Token.Lex(), err_Msg));
	}

	// Note: coercion best handled by a visitor - keep as is
	std::cout << "binOp1.Lex() = " << binOp1.Lex() << "\n";
	switch(binOp1.Tok()){
	case tok_plus: case tok_minus: case tok_div: case tok_mult:
	case tok_mod: 
	    LHS = new ArithmExpr_AST(binOp1, LHS, RHS);
	    break;
	case tok_log_or: case tok_log_and: case tok_log_eq: case tok_lt:
	case tok_log_ne: case tok_le: case tok_gt: case tok_ge:
	    LHS = new ArithmExpr_AST(binOp1, LHS, RHS); // TO DO: REPLACE***
	    break;
	default:
	    errExit(0, "illegal use of function parseInfixRHS (abort}");
	}
    }
}

Expr_AST*
parseParensExpr(void)
{
    std::cout << "parsing a ParensExpr...\n";
    getNextToken(); // create ready-state
    Expr_AST* E = parseExpr(0);

    if ( (-1 == match(0, tok_rdclosed, 1)) )
	throw(Punct_Error(')', 0));

    return E;
}

// Primary -> id | intVal | fltVal | (expr) | -expr
Expr_AST*
parsePrimaryExpr(void)
{
    std::cout << "parsing a Primary...: " << next_Token.Lex() << "\n";
    switch(next_Token.Tok()){
    case tok_intV: return parseIntExpr();
    case tok_doubleV: return parseFltExpr();
    case ';': getNextToken(); std::cout << "\n"; return parseExpr(0);
    case '(': return parseParensExpr();
    case '!': // return parseNotLogExpr();
    case '-': getNextToken(); return parseExpr(1);
    case tok_ID: // note: coming here, ID has already been entered into
	         //       the symbol table
	return parseIdExpr(next_Token.Lex());

    default: 
	throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));
	break;
    }
}

// block -> { [declaration]* [statement]* }
void
parseBlock(void)
{
    std::cout << "we should never be here...\n";
//    match(1, tok_paropen, 1);
    top_Env = addEnv(top_Env);
// to do
    top_Env = top_Env->getPrior();
//    match(0, tok_parclosed, 1);
}
