/********************************************************************
* parser.cpp - Parser for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*                (also known as Dijksta shunting algorithm) to flatten
*                the deep cfg
*
* Invariant: upon return, parse functions ensure to point ahead
*
* Mem Leaks: will add a function to delete AST*s and Env* later
*
********************************************************************/

#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"

token next_Token;
Node_AST* pFirst_Node;

token
getNextToken(void) { return (next_Token = getTok()); }

/***************************************
*  Primary expressions
***************************************/

int
match(int update_Prior, tokenType t, int update_Post)
{
    std::cout << "matching...\n";
    if (update_Prior) getNextToken();
    if ( (t == next_Token.Tok()) ){
	if (update_Post) getNextToken();
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
    std::cout << "\tparsing (retrieving) an Id...\n";
    // TO DO: once we also catch arrays/fcts, might need change
    Expr_AST* pId;
    if ( (0 == (pId = findNameInHierarchy(top_Env, Name))) )
	throw(Primary_Error(Name, "not declared"));

    if ( (0 == match(1, tok_rdopen, 0)) )
	; // TO DO: this can catch function definitions
    else if ( (0 == match(0, tok_sqopen, 0)) )
	; // TO DO: this can catch arrays
    else
	return pId;
    return 0; // to suppress gcc warning
}

int // 0: no coercion; 1: coerced LHS; 2: coerced RHS
checkForCoercion(Expr_AST* LHS, Expr_AST* RHS)
{
    if ( (LHS->TypeP() == RHS->TypeP()) )
	return 0;
    else if ( (LHS->TypeP() < RHS->TypeP()) )
	return 1;
    else
	return 2;
}

Expr_AST*
parseCoercion(Expr_AST* Expr, tokenType Type)
{
    Tmp_AST* pTmp = new Tmp_AST(token(Type));
    return new CoercedExpr_AST(pTmp, Expr);
}

Expr_AST* parsePrimaryExpr(void);
Expr_AST* parseInfixRHS(int, Expr_AST*);
int logOp_Tot = 0;

// 0 - infix; 1 - arithm prefix (-); 2 - logical prefix (!)
Expr_AST* 
parseExpr(int Infix)
{
    std::cout << "parsing an expr...\n";
    Expr_AST* LHS = parsePrimaryExpr();
    if ( !LHS )
	throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));

    Expr_AST* ptmp_AST = 0;
    if ( (1 == Infix) )
	ptmp_AST = new UnaryArithmExpr_AST(token(tok_minus), LHS);
    else if ( (2 == Infix) )
	ptmp_AST = new NotExpr_AST(token(tok_log_not), LHS);
    if ( (0 == ptmp_AST) )
	return parseInfixRHS(0, LHS); // don't impose any precedence on LHS
    else
	return parseInfixRHS(0, ptmp_AST);
    return 0; // to suppress gcc warning

}

Expr_AST* 
dispatchExpr(void)
{
    switch(next_Token.Tok()){
    case '!': 
	getNextToken(); 
	return parseExpr(2);
    case '-':
	getNextToken();
	return parseExpr(1);
    default:
	return parseExpr(0);
    }
}

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
    std::string const err_Msg = "Expected primary expression";
    std::string const err_Msg2 = "illegal chaining of logical operators";
    for (;;){
	int prec_2 = opPriority(next_Token.Tok());
	std::cout << "current token (1) = " << next_Token.Lex() << "\n";

	if ( (prec_2 < prec_1) )
	    return LHS;
	token binOp1 = next_Token; // store it for later op creation

	getNextToken();
	Expr_AST* RHS = parsePrimaryExpr();
	if (!RHS)
	    throw(Primary_Error(next_Token.Lex(), err_Msg));

	int prec_3 = opPriority(next_Token.Tok());
	std::cout << "current token (2) = " << next_Token.Lex() << "\n";

	if (prec_2 < prec_3){ // flip from l-r, to r-l (at least for one step)
	    RHS = parseInfixRHS(prec_2 + 1, RHS); // keep going l-r until 
	    if (!RHS)
		throw(Primary_Error(next_Token.Lex(), err_Msg));
	}

	std::cout << "binOp1.Lex() = " << binOp1.Lex() << "\n";
	int tmp = checkForCoercion(LHS, RHS);
	if ( (1 == tmp) ){
	    std::cout << "coercing LHS...\n";
	    LHS = parseCoercion(LHS, RHS->Type().Tok());
	}
	else if ( (2 == tmp) ){
	    std::cout << "coercing RHS...\n";
	    RHS = parseCoercion(RHS, LHS->Type().Tok());
	} 
	switch(binOp1.Tok()){
	case tok_plus: case tok_minus: case tok_div: case tok_mult:
	case tok_mod: 
	    LHS = new ArithmExpr_AST(binOp1, LHS, RHS);
	    break;
	case tok_log_or: 
	    LHS = new OrExpr_AST(LHS, RHS);
	    break;
	case tok_log_and:
	    LHS = new AndExpr_AST(LHS, RHS);
	    break;
	case tok_log_eq: case tok_log_ne: case tok_lt:
	case tok_le: case tok_gt: case tok_ge:
	    if ( (1 < ++logOp_Tot) )
		throw(Primary_Error(binOp1.Lex(), err_Msg2));
	    LHS = new RelExpr_AST(binOp1, LHS, RHS);
	    break;
	default:
	    errExit(0, "illegal use of function parseInfixRHS (abort}");
	}
    }
}

Expr_AST*
parseParensExpr(void)
{
    int oldLogic_Status = logOp_Tot;
    logOp_Tot = 0;
    std::cout << "parsing a ParensExpr...\n";
    getNextToken(); // create ready-state
    Expr_AST* E = dispatchExpr();

    if ( (-1 == match(0, tok_rdclosed, 1)) )
	throw(Punct_Error(')', 0));

    logOp_Tot = oldLogic_Status;

    return E;
}

// ***TO DO: how to best hand off calculated value?
// Primary -> id | intVal | fltVal | (expr) | -expr
Expr_AST*
parsePrimaryExpr(void)
{
    std::cout << "parsing a Primary...: " << next_Token.Lex() << "\n";
    switch(next_Token.Tok()){
    case tok_intV: return parseIntExpr();
    case tok_doubleV: return parseFltExpr();
    case ';': 
	getNextToken();
	std::cout << "\n";
	logOp_Tot = 0;
	return parseExpr(0);
    case '(': return parseParensExpr(); 
    case '!': 
    case '-':
	return dispatchExpr();
    case tok_ID: // coming here, ID has already been entered into ST
	return parseIdExpr(next_Token.Lex());
    default: 
	throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));
	break;
    }
}

// decl -> type id;
//         type id = expr; (currently no basic ctor - easily added)
Decl_AST* 
parseVarDecl(token Type)
{
    // access error
    if ( !(tok_ID == next_Token.Tok()) )
	throw (Primary_Error(next_Token.Lex(), "expected primary Identifier"));
    if ( (0 == findNameInHierarchy(top_Env, next_Token.Lex())) )
	throw(Redefine_Error(next_Token.Lex()));

    // handle arrays
    token op_Token = next_Token;
    getNextToken();
    if ( (0 == match(1, tok_sqopen, 0)) )
	; // ****TO DO: this can catch arrays****

    IdExpr_AST* new_Id;
    Expr_AST* RHS;
    switch(next_Token.Tok()){
    case tok_eq: 
	getNextToken();
	new_Id = new IdExpr_AST(Type, op_Token);
	top_Env->insertName(op_Token.Lex(), new_Id);
	RHS = dispatchExpr(); 
	if ( (0 == RHS) )
	    throw(Primary_Error(op_Token.Lex(), "invalid initialization"));
	break;
    case tok_semi:
	getNextToken();
	new_Id = new IdExpr_AST(Type, op_Token);
	top_Env->insertName(op_Token.Lex(), new_Id);
	RHS = 0;
	break;
    default: 
	throw(Primary_Error(next_Token.Lex(), "expected = or ; instead"));
    }

    Decl_AST* ret = new Decl_AST(new_Id, RHS);
    getNextToken();
    return ret;
}


// stmtLst -> { [varDecl | stmt]* }
// ***TO DO: VERY preliminary (among others, needs re-thinking of errors)***
// as set up, just to test one { ... } block
void
parseStmtLst(void)
{
    match(1, tok_paropen, 1);
    top_Env = addEnv(top_Env); // ***TO DO: REMOVE. Better elsewhere.

    while ( (tok_eof != next_Token.Tok()) ){

	switch(next_Token.Tok()){
	case tok_int:
	    getNextToken();
	    parseVarDecl(token(tok_int));
	    break;
	case tok_double:
	    getNextToken();
	    parseVarDecl(token(tok_double));
	    break;
	default:
	    break;
	}
    }

    top_Env = top_Env->getPrior(); // ***TO DO: REMOVE. Better elsewhere.
    match(0, tok_parclosed, 1);
}
