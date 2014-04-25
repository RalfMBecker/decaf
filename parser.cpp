/********************************************************************
* parser.cpp - Parser for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*                (also known as Dijksta shunting algorithm) to flatten
*                the deep cfg
*
* Invariant: upon return, parse functions ensure to point ahead
*
* Error handling: local error report, after which the error is 
*                 handed on to statement level, and processed there
*                 (using Panic Mode recovery) 
*
* Mem Leaks: will add a function to delete AST*s and Env* later
*
********************************************************************/
// **TO DO: consider keeping debugging output with a -v (verbose) switch

#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"

Node_AST* pFirst_Node;
extern std::istream* input;
extern int errorIn_Progress;

int frame_Depth = 0; // track depth of scope nesting
int if_Active = 0;

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
	for (i = 0; i > N; i--){	    
	    if ( (0 == top_Env) || (root_Env == top_Env) ){
		parseWarning("error processing", "symbol table corrupted"); 
		return;
	    }
	    top_Env = top_Env->getPrior();
	}
    }
}

Stmt_AST*
errorResetStmt(void)
{
    adjScopeLevel(panicModeFwd());
    errorIn_Progress = 0;
    return 0;
}

int
match(int update_Prior, tokenType t, int update_Post)
{
    std::cout << "matching...\n";
    if (errorIn_Progress) return -2;

    if (update_Prior) getNextToken();
    if (errorIn_Progress) return -2;
    if ( (t == next_Token.Tok()) ){
	if (update_Post) getNextToken();
	if (errorIn_Progress) return -2;
	return 0;
    }
    else
	return -1;
}

/***************************************
*  Primary expressions (- switch board)
***************************************/
// int -> <integer value>
Expr_AST*
parseIntExpr(void)
{
    std::cout << "parsing an int...\n";

    Expr_AST* res = new IntExpr_AST(next_Token);
    getNextToken();
    if (errorIn_Progress) return 0;
    return res;
}

// double -> <double value>
Expr_AST*
parseFltExpr(void)
{
    std::cout << "parsing a flt...\n";

    Expr_AST* res = new FltExpr_AST(next_Token);
    getNextToken();
    if (errorIn_Progress) return 0;
    return res;
}

// ******TO DO: NEEDS EXTENSION ONCE FUNCTIONS ALLOWED***********
// id -> alpha* [alphanum | _]*
// Used for *reading an ID*, not its definition
// Disambiguates shadowed names; returning the currently active one.
Expr_AST*
parseIdExpr(std::string Name)
{
    std::cout << "\tparsing (retrieving) an Id...\n";

    // TO DO: once we also catch arrays/fcts, might need change
    Expr_AST* pId;
    if ( (0 == (pId = findNameInHierarchy(top_Env, Name))) ){
	varAccessError(next_Token.Lex(), 0);
	errorIn_Progress = 1;
	return 0;
    }

    if ( (0 == match(1, tok_rdopen, 0)) ){
	if (errorIn_Progress) return 0;
	; // TO DO: this can catch function definitions
    }
    else if ( (0 == match(0, tok_sqopen, 0)) )
	; // TO DO: this can catch arrays
    else
	return pId;
    if (errorIn_Progress) return 0;

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

/***********************************************
* Expressions functions (+ primary switchboard)
***********************************************/
Expr_AST* parsePrimaryExpr(void);
Expr_AST* parseInfixRHS(int, Expr_AST*);
int logOp_Tot = 0;

// expr -> prim op expr | -expr | !expr | prim | epsilon
// op -> +, -, *, /, %, ||, &&, op1
// op1 -> ==, !=, <, <=, >, >= 
// Type: 0 - infix; 1 - arithm prefix (-); 2 - logical prefix (!)
// logOp_Tot: helps tracking that only 1 op1 type is valid in each expr 
Expr_AST* 
parseExpr(int Type)
{
    std::cout << "parsing an expr...\n";

    logOp_Tot = 0;
    Expr_AST* LHS = parsePrimaryExpr();
    if ( !LHS )
	return 0; // handled by caller

    Expr_AST* ptmp_AST = 0;
    if ( (1 == Type) )
	ptmp_AST = new UnaryArithmExpr_AST(token(tok_minus), LHS);
    else if ( (2 == Type) )
	ptmp_AST = new NotExpr_AST(token(tok_log_not), LHS);
    else
	ptmp_AST = LHS;

    if (errorIn_Progress) return 0;
    return parseInfixRHS(0, ptmp_AST);
}

Expr_AST* 
dispatchExpr(void)
{
    if (errorIn_Progress) return 0;

    switch(next_Token.Tok()){
    case '!': 
	getNextToken(); 
	if (errorIn_Progress) return 0;
	return parseExpr(2);
    case '-':
	getNextToken();
	if (errorIn_Progress) return 0;
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
// The way we track recursion, could go bad for deeply recursive infix (> 99).
// Restriction should be added to a full description of the compiler/language.

Expr_AST* 
parseInfixRHS(int prec_1, Expr_AST* LHS)
{ 
    std::cout << "entering parseInfixRHS...\n";

    std::string const err_Msg = "Expected primary expression";
    std::string const err_Msg2 = "illegal chaining of logical operators";
    std::string const err_Msg3 = "illegal assignment: lvalue expected";
    for (;;){
	int prec_2 = opPriority(next_Token.Tok());
	std::cout << "current token (1) = " << next_Token.Lex() << "\n";

	if ( (prec_2 < prec_1) )
	    return LHS;
	token binOp1 = next_Token; // store it for later op creation

	getNextToken();
	if (errorIn_Progress) return 0;
	Expr_AST* RHS = parsePrimaryExpr();
	if ( (!RHS) || (errorIn_Progress) ){ // double check not necessary
	    parseError(next_Token.Lex(), err_Msg);
	    errorIn_Progress = 1;
	    return 0;
	}

	int prec_3 = opPriority(next_Token.Tok());
	std::cout << "current token (2) = " << next_Token.Lex() << "\n";

	// Note: as we next read a Primary_Expr (which dispatches '-', 
	//       '!'), this handles a prefix expr following fine.
	if (prec_2 < prec_3){ // flip from l-r, to r-l (at least for one step)
	    RHS = parseInfixRHS(prec_2 + 1, RHS); // keep going l-r until 
	    if ( (!RHS) || (errorIn_Progress) ){
		parseError(next_Token.Lex(), err_Msg);
		errorIn_Progress = 1;
		return 0;
	    }
	}

	std::cout << "binOp1.Lex() = " << binOp1.Lex() << "\n";
	int is_Assign = (tok_eq == binOp1.Tok())?1:0;
	if ( (is_Assign) && !(dynamic_cast<IdExpr_AST*>(LHS)) ){
	    parseError(next_Token.Lex(), err_Msg3);
	    errorIn_Progress = 1;
	    return 0;
	}

	int tmp = checkForCoercion(LHS, RHS);
	if ( (1 == tmp) && !(is_Assign) ){
	    std::cout << "coercing LHS...\n";
	    LHS = parseCoercion(LHS, RHS->Type().Tok());
	}
	else if ( (2 == tmp) || ( (1 == tmp) && (is_Assign) ) ){
	    std::cout << "coercing RHS...\n";
	    RHS = parseCoercion(RHS, LHS->Type().Tok());
	} 

	switch(binOp1.Tok()){
	case tok_plus: case tok_minus: case tok_div: case tok_mult:
	case tok_mod: 
	    LHS = new ArithmExpr_AST(binOp1, LHS, RHS);
	    break;
	case tok_eq: // validity check above
	    LHS = new AssignExpr_AST(dynamic_cast<IdExpr_AST*>(LHS), RHS);
	    break;
	case tok_log_or: 
	    LHS = new OrExpr_AST(LHS, RHS);
	    break;
	case tok_log_and:
	    LHS = new AndExpr_AST(LHS, RHS);
	    break;
	case tok_log_eq: case tok_log_ne: case tok_lt:
	case tok_le: case tok_gt: case tok_ge:
	    if ( (1 < ++logOp_Tot) ){
		parseError(binOp1.Lex(), err_Msg2);
		errorIn_Progress = 1;
		return 0;
	    }
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
    std::cout << "parsing a ParensExpr...\n";

    if ( (-1 == match(0, tok_rdopen, 1)) ){
	punctError(')', 0);
	errorIn_Progress = 1;
	return 0;
    }

    int oldLogic_Status = logOp_Tot;
    logOp_Tot = 0;
    Expr_AST* E = dispatchExpr();

    if ( (-1 == match(0, tok_rdclosed, 1)) ){
	punctError(')', 0);
	errorIn_Progress = 1;
	return 0;
    }
    logOp_Tot = oldLogic_Status;

    return E;
}

// Primary -> id | intVal | fltVal | (expr) | -expr | !expr
Expr_AST*
parsePrimaryExpr(void)
{
    std::cout << "parsing a Primary...: " << next_Token.Lex() << "\n";
    if (errorIn_Progress) return 0;

    switch(next_Token.Tok()){
    case tok_ID: // coming here, ID should be in symbol table
	return parseIdExpr(next_Token.Lex());
    case tok_intV: return parseIntExpr();
    case tok_doubleV: return parseFltExpr();
    case '(': return parseParensExpr(); 
    case '=':
    case '-':
    case '!': 
	return dispatchExpr();
    case ';': // might terminate an empty expression; don't forward
	return 0; // caller must handle properly
    default: // reporting handled by caller 
	// parseError(next_Token.Lex(), "expected primary expression");
	errorIn_Progress = 1; // fall through for return
	break;
    }
    return 0;
}

/***************************************
*  Statement parsing functions
***************************************/
StmtList_AST* parseBlock();
Stmt_AST* parseStmt();

Block_AST*
dispatchStmt(void)
{
    Block_AST* stmt;
    if ( (0 == match(0, tok_paropen, 0)) ){
	stmt = parseBlock();
	if (errorIn_Progress) stmt = 0;
    }
    else{
	top_Env = addEnv(top_Env);
	frame_Depth++;
	stmt = parseStmt();
	top_Env = top_Env->getPrior();
	frame_Depth--;
	if (errorIn_Progress) stmt = 0;
    }

    return stmt;
}

// (if_stmt) stmt -> if (expr) [ stmt | block ] // stmt is block; for clarity
// if_stmt [ epsilon | else [ stmt | block ] | if_stmt ]
// invariant: - upon entry, points at tok_if
// Type: 0 - leading if; 1 - else if
If_AST* 
parseIfStmt(int Type)
{
    std::cout << "parsing an if statement...\n";

    if ( (-1 == match(0, tok_if, 1)) )
	errExit(0, "parseIfStmt should be called pointing at tok_if\n");

    Expr_AST* expr = parseParensExpr();
    if (errorIn_Progress) return 0;

    // handle [ stmt | block ]
    Block_AST* block = dispatchStmt();
    if (errorIn_Progress) return 0;

    int hasElse = 0;
    int endBlock_Marker = 0;
    if ( (tok_else == next_Token.Tok()) )
	hasElse = 1;
    else if ( (tok_parclosed == next_Token.Tok()) ){
	if_Active -= (if_Active > 0)?1:0;
	endBlock_Marker = 1;
    }
    If_AST* pIf = new If_AST(expr, block, Type, hasElse, endBlock_Marker);

    return pIf;
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
    if ( (tok_ID != next_Token.Tok()) ){
	parseError(next_Token.Lex(), "expected primary identifier");
	errorIn_Progress = 1;
	return 0;
    }
    Env* prior_Env = findFrameInHierarchy(top_Env, next_Token.Lex());
    if ( (prior_Env == top_Env) ){
	varAccessError(next_Token.Lex(), 1);
	errorIn_Progress = 1;
	return 0;
    }

    // handle arrays
    token op_Token = next_Token;
    if ( (0 == match(1, tok_sqopen, 0)) ){
	if (errorIn_Progress) return 0;
	; // ****TO DO: this can catch arrays****
    }

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
	if (errorIn_Progress) return 0;
	break;
    case tok_eq: // prepare to call parseAssign() next
	putBack('=');
	next_Token = op_Token;
	break;
    default: 
	parseError(next_Token.Lex(), "exptected \'=\' or \';\' instead");
	errorIn_Progress = 1;
	return 0;
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
    if ( (0 == LHS) ){
	// varAccessError(next_Token.Lex(), 0); report at IdExpr level
	errorIn_Progress = 1;
	return 0;
    }
    // **TO DO: allow for array access too

    Expr_AST* RHS;
    switch(next_Token.Tok()){
    case ';': 
	RHS = 0;
	break;
    case '=':
	getNextToken();
	if (errorIn_Progress) return 0;
	RHS = dispatchExpr();
	if ( (0 == RHS) ){
	    parseError(next_Token.Lex(), "invalid assignment");
	    errorIn_Progress = 1;
	    return 0;
	}
	if ( (-1 == match(0, tok_semi, 0)) ){
	    punctError(';', 0);
	    errorIn_Progress = 1;
	    return 0;
	}
	break;
    default:
	parseError(next_Token.Lex(), "\'=\' or \';\' expected");
	errorIn_Progress = 1;
	return 0;
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
    if (errorIn_Progress) return 0;

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

    Stmt_AST* ret;

    switch(next_Token.Tok()){
    case tok_int:
	getNextToken();
	if (errorIn_Progress) return errorResetStmt();
	ret = parseVarDecl(token(tok_int));
	if (errorIn_Progress) return errorResetStmt();
	break;
    case tok_double:
	getNextToken();
	if (errorIn_Progress) return errorResetStmt();
	ret = parseVarDecl(token(tok_double));
	if (errorIn_Progress) return errorResetStmt();
	break;
    case tok_ID: 
	ret = parseAssign();
	if (errorIn_Progress) return errorResetStmt();
	break;
    case tok_if:
	ret = parseIfStmt(0);
	if (errorIn_Progress) return errorResetStmt();
	if_Active++;
	break;
    case tok_else:
	if ( !(if_Active) ){
	    parseError(next_Token.Lex(), "else without prior if");
	    errorResetStmt(); //**TO DO: can be handled better
	    return 0;
	}

	if ( (-1 == match(1, tok_if, 0)) ){ // terminating else case
	    if_Active--;
	    if (errorIn_Progress) return errorResetStmt();
	    Block_AST* tmp = dispatchStmt();
	    if (errorIn_Progress) return 0;
	    int endBlock_Marker = 0;
	    if ( (tok_parclosed == next_Token.Tok()) ){
		if_Active -= (if_Active > 0)?1:0;
		endBlock_Marker = 1;
	    }
	    ret = new Else_AST(tmp, endBlock_Marker);
	}
	else{ // else if case (no impact on if_Active level)
	    if (errorIn_Progress) return errorResetStmt();
	    ret = parseIfStmt(1);
	    if (errorIn_Progress) return errorResetStmt();
	}
	break;
    default: // assume empty expression
	parseWarning("", "unused expression");
	dispatchExpr(); // parse and discard
	if (errorIn_Progress) return errorResetStmt();
	if ( (-1 == match(0, tok_semi, 1)) ){
	    punctError(';', 0);
	    return errorResetStmt();
	}
	return 0;
	break;
    }

    return ret;
}

StmtList_AST* parseStmtList(void);
StmtList_AST* parseStmtListCtd(StmtList_AST*);

// **TO DO: problem if the following combination: (a) 1-line stmtList (if we 
//  have several stmts with no {}, this applies to them all, and (2) var
//  declarations in those lines, used later in the line (no new scope prepared
//  in current drafting)
// block -> { [stmtList | { stmtList }]* } 
// invariants -> entering, point at '{', if any
StmtList_AST*
parseBlock(void)
{
    std::cout << "parsing a block...\n";

    StmtList_AST* pSL;
    if ( (-1 == match(0, tok_paropen, 1)) )
	return 0; // **TO DO: add error handling on this level
    if ( (0 == match(0, tok_parclosed, 0)) ){
	getNextToken();
	return new StmtList_AST(0, 0);
    }

    top_Env = addEnv(top_Env);
    frame_Depth++;

    pSL = parseStmtList();

    // TO DO: more thinking about this once integrated higher
    if ( (0 < frame_Depth) ){ // could have been reduced in error handling
	int test;
	if ( (-1 == (test = match(0, tok_parclosed, 0))) ){
	    punctError('}', 0);
	    // errorIn_Progress = 1; **TO DO: handle later
	    pSL = 0;
	}
	else if ( (-2 == test) ){
	    // errorIn_Progress = 1; **TO DO: handle later
	    pSL = 0;
	}
	else{
	    top_Env = top_Env->getPrior();
	    frame_Depth--;
	}
    }

    if ( (tok_eof != next_Token.Tok()) ) getNextToken();
    if (errorIn_Progress) return 0;

    return pSL;
}

// stmtLst -> { [stmt stmtLst] } | stmt | epsilon 
// Note: errors handled on level below; so we are clean here
StmtList_AST*
parseStmtList(void)
{
    std::cout << "parsing a stmtList...\n";

    Stmt_AST* LHS = parseStmt();
    if ( (0 < frame_Depth) ) // could be less if error
	return parseStmtListCtd(LHS); // points ahead
    else
	return LHS;
}

// for logic, compare parseInfixExpr() - similar, with path determined by
// hitting '{' and '}'
// invariant: - upon entry, we point onto the first token of the next stmt
//            - upon return, points to '}'
StmtList_AST*
parseStmtListCtd(StmtList_AST* LHS)
{
    std::cout << "entering parseStmtListCtd...\n";

    if ( (1 > frame_Depth) || (tok_eof == next_Token.Tok()) )
	errExit(0, "missing \'}\' - symbol table corrupted");

    StmtList_AST* RHS;
    for (;;){
	switch(next_Token.Tok()){
	case '{':
	    RHS = parseBlock();
	    if (!RHS) return 0;
	    break;
	case '}': // if we skip by one in errorResetStmt(), we get stuck. 
	    if ( (0 < frame_Depth) )
		return LHS;
	    else
		errExit(0, "spare '}' - symbol table corrupted");
	    break;
	default:
	    RHS = parseStmt(); // RHS = 0 signals (1) empty expr; (2) error
	    if ( (0 < frame_Depth) )
		RHS = parseStmtListCtd(RHS);
	    else
		errExit(0, "spare '}' - symbol table corrupted");
	    break;
	}

	LHS = new StmtList_AST(LHS, RHS);
    } // before call of this fct                    after
      //        LHS_b                               LHS_a
      //                                         LHS_b   RHS (compound)

    return 0; // to suppress gcc warning
}
