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
// **TO DO: consider keeping debugging output with a -v (verbose) switch

#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"

token next_Token;
Node_AST* pFirst_Node;
extern std::istream* input;

int frame_Depth = 0; // track depth of scope nesting

/***************************************
*  Helper functions
***************************************/

// used in error handling (fed by rv of panicModeFwd())
void
adjScopeLevel(int N)
{
    int i;

    frame_Depth += N;
    if ( (0 < N) ){
	for (i = 0; i < N; i++)
	    top_Env = addEnv(top_Env);
    }
    else{
	for (i = 0; i > N; i--)
	    top_Env = top_Env->getPrior();
    }
}

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

// int -> <integer value>
Expr_AST*
parseIntExpr(void)
{
    std::cout << "parsing an int...\n";
    Expr_AST* res = new IntExpr_AST(next_Token);
    getNextToken();
    return res;
}

// double -> <double value>
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
// id -> alpha* [alphanum | _]*
// Used for *reading an ID*, not its definition
// Disambiguates shadowed names; returning the currently active one.
Expr_AST*
parseIdExpr(std::string Name)
{
    std::cout << "\tparsing (retrieving) an Id...\n";
    // TO DO: once we also catch arrays/fcts, might need change
    Expr_AST* pId;
    if ( (0 == (pId = findNameInHierarchy(top_Env, Name))) )
	; // throw(Primary_Error(Name, "not declared"));

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

// cast (id, type) -> tmp
Expr_AST*
parseCoercion(Expr_AST* Expr, tokenType Type)
{
    Tmp_AST* pTmp = new Tmp_AST(token(Type));
    return new CoercedExpr_AST(pTmp, Expr);
}

Expr_AST* parsePrimaryExpr(void);
Expr_AST* parseInfixRHS(int, Expr_AST*);
int logOp_Tot = 0;

// expr -> prim op expr | -expr | !expr | prim | epsilon
// op -> +, -, *, /, %, ||, &&, op1
// op1 -> ==, !=, <, <=, >, >= 
// 0 - infix; 1 - arithm prefix (-); 2 - logical prefix (!)
// logOp_Tot: helps tracking that only 1 op1 type is valid in each expr 
Expr_AST* 
parseExpr(int Infix)
{
    std::cout << "parsing an expr...\n";
    logOp_Tot = 0;
    Expr_AST* LHS = parsePrimaryExpr();
    if ( !LHS )
	; // throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));

    Expr_AST* ptmp_AST = 0;
    if ( (1 == Infix) )
	ptmp_AST = new UnaryArithmExpr_AST(token(tok_minus), LHS);
    else if ( (2 == Infix) )
	ptmp_AST = new NotExpr_AST(token(tok_log_not), LHS);
    else
	ptmp_AST = LHS;

    return parseInfixRHS(0, ptmp_AST);
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
// Example to illustrate variable usage: LHS + b * c - d
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
	    ; // throw(Primary_Error(next_Token.Lex(), err_Msg));

	int prec_3 = opPriority(next_Token.Tok());
	std::cout << "current token (2) = " << next_Token.Lex() << "\n";

	// Note: as we next read a Primary_Expr (which dispatches '-', 
	//       '!'), this handles a prefix expr following fine.
	if (prec_2 < prec_3){ // flip from l-r, to r-l (at least for one step)
	    RHS = parseInfixRHS(prec_2 + 1, RHS); // keep going l-r until 
	    if (!RHS)
		; // throw(Primary_Error(next_Token.Lex(), err_Msg));
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
		; // throw(Primary_Error(binOp1.Lex(), err_Msg2));
	    LHS = new RelExpr_AST(binOp1, LHS, RHS);
	    break;
	default:
	    errExit(0, "illegal use of function parseInfixRHS (abort}");
	}
    }
}

// (expr) -> expr
Expr_AST*
parseParensExpr(void)
{
    int oldLogic_Status = logOp_Tot;
    logOp_Tot = 0;
    std::cout << "parsing a ParensExpr...\n";
    getNextToken(); // create ready-state
    Expr_AST* E = dispatchExpr();

    if ( (-1 == match(0, tok_rdclosed, 1)) )
	; // throw(Punct_Error(')', 0));

    logOp_Tot = oldLogic_Status;

    return E;
}

// Primary -> id | intVal | fltVal | (expr) | -expr | !expr
Expr_AST*
parsePrimaryExpr(void)
{
    std::cout << "parsing a Primary...: " << next_Token.Lex() << "\n";
    switch(next_Token.Tok()){
    case tok_ID: // coming here, ID has already been entered into ST
	return parseIdExpr(next_Token.Lex());
    case tok_intV: return parseIntExpr();
    case tok_doubleV: return parseFltExpr();
    case '(': return parseParensExpr(); 
    case '-':
    case '!': 
	return dispatchExpr();
    default: 
	; // throw(Primary_Error(next_Token.Lex(), "Expected primary expression"));
	break;
    }
    return 0; // to suppress gcc warning
}

// decl -> type id;
//         type id = expr; (currently no basic ctor - easily added)
// Shadowing: allowed
// invariant: - exiting, we confirm ';' (decl), or move back to Id (dec+init)
//            - entering, points at Id
// Note: if we initialize, prepare to call parseAssign() after
VarDecl_AST* 
parseVarDecl(token Type)
{
    std::cout << "parsing a var declaration...\n";
    // access error (allow for shadowing)
    if ( (tok_ID != next_Token.Tok()) )
	; // throw (Primary_Error(next_Token.Lex(), "expected primary identifier"));
    Env* prior_Env = findFrameInHierarchy(top_Env, next_Token.Lex());
    if ( (prior_Env == top_Env) )
	; // throw(VarAccess_Error(next_Token.Lex(), 1));

    // handle arrays
    token op_Token = next_Token;
    if ( (0 == match(1, tok_sqopen, 0)) )
	; // ****TO DO: this can catch arrays****

    // declare new Id object even if we later see a syntax error (this is fine)
    IdExpr_AST* new_Id;
    new_Id = new IdExpr_AST(Type, op_Token);
    int err_Code;
    const char e_M[50] = "cannot insert \"%s\" into symbol table (code %d)";
    // **TO DO: monitor if also for heap***
    if ( (0!= (err_Code = addIdToEnv(top_Env, new_Id, "stack"))) )
	errExit(0, e_M , new_Id->Addr().c_str(), err_Code);

    switch(next_Token.Tok()){
    case tok_semi: // we are done - declaration only
	getNextToken();
	break;
    case tok_eq: // prepare to call parseAssign() next
	input->putback('=');
	next_Token = op_Token;
	break;
    default: 
	; // throw(Primary_Error(next_Token.Lex(), "expected = or ; instead"));
    }

    return new VarDecl_AST(new_Id);
}

// assign -> idExpr [= Expr; | ; ] 
// invariant: - upon exit, guarantees that ';' terminates
//            - upon entry, points at id
Assign_AST* 
parseAssign(void)
{
    std::cout << "parsing an assignment...\n";

    // access error (**TO DO: handle arrays too)
    Expr_AST* LHS = parseIdExpr(next_Token.Lex());
    if ( (0 == LHS) )
	; // throw(VarAccess_Error(next_Token.Lex(), 0));
    // **TO DO: allow for array access too

    Expr_AST* RHS;
    switch(next_Token.Tok()){
    case ';': 
	RHS = 0;
	break;
    case '=':
	getNextToken();
	RHS = dispatchExpr();
	if ( (0 == RHS) )
	    ; // throw(Primary_Error(next_Token.Lex(), "invalid assignment"));
	if ( (-1 == match(0, tok_semi, 0)) )
	    ; // throw(Punct_Error(';', 0));
	break;
    default:
	; // throw(Primary_Error(next_Token.Lex(), "= or ; expected"));
	break;
    }

    if ( (0 != RHS) ){
	if ( (LHS->Type().Tok() != RHS->Type().Tok()) )
	    RHS = parseCoercion(RHS, LHS->Type().Tok());
    }
    if ( !(dynamic_cast<IdExpr_AST*>(LHS)) ) // in case we coerced
	errExit(0, "logical flaw in use of function parseAssign");
    Assign_AST* ret = new Assign_AST(dynamic_cast<IdExpr_AST*>(LHS), RHS);
    getNextToken();

    return ret;
}

// stmt    -> [ varDecl | expr | if-stmt | while-stmt | epsilon ]
// invariant -> leaving, guarantees a ';' has been found where needed;
//              entering, next_Token=first token of stmt, which is
//              guaranteed to be not '{' or '}'
Stmt_AST*
parseStmt(void)
{
    std::cout << "parsing a statement...\n";
    token t = next_Token;
    Stmt_AST* ret;

    switch(t.Tok()){
    case tok_int:
	getNextToken();
	ret = parseVarDecl(token(tok_int));
	break;
    case tok_double:
	getNextToken();
	ret = parseVarDecl(token(tok_double));
	break;
    case tok_ID: 
	ret = parseAssign();
	break;
    default: // assume empty expression
	// ** TO DO: proper function
	std::cerr << "Near " <<  line_No << ":" << col_No << ": ";
	std::cerr << "warning - unused expression\n";
	dispatchExpr(); // parse and discard
	if ( (-1 == match(0, tok_semi, 1)) )
	    ; // throw(Punct_Error(';', 0));
	return 0;
	break;
    }

    return ret;
}

// ****TO DO: relocate/re-order functions
StmtList_AST* parseStmtList(void);
StmtList_AST* parseStmtListCtd(StmtList_AST* LHS);

// block -> { [stmtList | { stmtList }]* } 
// invariants -> entering, point at '{', if any
StmtList_AST*
parseBlock(void)
{
    std::cout << "parsing a block...\n";

    StmtList_AST* pSL;
    if ( (0 == match(0, tok_paropen, 0)) ){
	if ( (0 == match(1, tok_parclosed, 0)) ){
	    getNextToken();
	    return new StmtList_AST(0, 0);
	}
	top_Env = addEnv(top_Env);
	frame_Depth++;
    }

    pSL = parseStmtList();

    if ( (0 < frame_Depth) ){
	if ( (-1 == match(0, tok_parclosed, 1)) )
	    ; // throw(Punct_Error('}', 0));
	top_Env = top_Env->getPrior();
	frame_Depth--;
    }

    return pSL;
}

// stmtLst -> { [stmt stmtLst] } | stmt | epsilon 
StmtList_AST*
parseStmtList(void)
{
    std::cout << "parsing a stmtList...\n";

    Stmt_AST* LHS = parseStmt();
    return parseStmtListCtd(LHS); // points ahead
}

// for logic, compare parseInfixExpr() - similar, with path determined by
// hitting '{' and '}'
// invariant: - upon entry, we point onto the first token of the next stmt
//            - upon return, points to '}'
StmtList_AST*
parseStmtListCtd(StmtList_AST* LHS)
{
    std::cout << "entering parseStmtListCtd...\n";
    std::string const err_Msg = "expected statement";
    StmtList_AST* RHS;
    for (;;){
	switch(next_Token.Tok()){
	case '{':
	    RHS = parseBlock();
	    if (!RHS)
		; // throw(Primary_Error(next_Token.Lex(), err_Msg));
	    break;
	case '}':
	    return LHS;
	    break;
	default:
	    RHS = parseStmt();
	    // **TO DO: check for 0 - any actions?
	    RHS = parseStmtListCtd(RHS);
	    break;
	}
	LHS = new StmtList_AST(LHS, RHS);
    } // before call of this fct                    after
      //        LHS_b                               LHS_a
      //                                         LHS_b   RHS (compound)
    return 0; // to suppress gcc warning
}
