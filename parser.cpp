/********************************************************************
* parser.cpp - Parser for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*                (also known as Dijksta shunting algorithm) to flatten
*                the deep cfg
*
* Invariant: upon return, parse functions guarantee to point ahead
*
* Error handling: local error report, after which the error is 
*                 handed on to statement level, and processed there
*                 (using Panic Mode recovery) 
* Note (errors): Warning in '='/'==' case for 'if' is off by a line
*
* Notes on semantic structure handling:
* a++/++a: Global variables:
*          Global tables prefix_Table/postfix_Table keep track, for each
*          variable, how many pre- and post-increment operations during 
*          the parsing of one (compound) expression have been seen.
* 
*          ast.h:
*          Expr_AST has been enhanced with private versions of these 
*          globals, which are generally empty.
*
*          parser.cpp 
*          In case the expression actually is a statement of form
*          "a++"; or "--a;", copy the globals into the privates
*          of an IncrIdExpr_AST type (in parseAssignStmt() and
*          parseStmt()). Clear the globals after.
*          In case of a compound expression E, copy the globals which
*          track pre- and post-increments in all sub-expressions to
*          the private version of the top level E only (in dispatchExrp()). 
*
*          visitor.h/philosophy:
*          Operators are only allowed in expressions (not as lvalues).
*          If E is an expression having sub-expressions with the above
*          tables not empty, generate SSA as follows, where i, j are
*          the sum of adjustements resulting from pre-/post-increments:
*          print(prefix_Table) SSAs ("+ a a i")
*          print E SSAs
*          print(postfix_Table) SSAs ("+ a a j")
*          This means we actually have *no undefined behavior*.
*
********************************************************************/

#include <vector>
#include <cstdlib>
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"

Node_AST* pFirst_Node;
extern std::istream* input;
extern int errorIn_Progress;

extern int option_Debug;
int frame_Depth = 0; // track depth of scope nesting (used in error handling)
int break_Enabled = 0;
int emitRtError_Section = 0; // if we encounter an array declaration whose
// dimensions are integer expressions, we prepare an error handling environment
// at run-time (at IR level).

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
    if (option_Debug) std::cerr << "\trecovering from error...\n";

    adjScopeLevel(panicModeFwd());
    errorIn_Progress = 0;
    return 0;
}

int
match(int update_Prior, tokenType t, int update_Post)
{
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
    if (option_Debug) std::cout << "parsing an int...\n";

    Expr_AST* res = new IntExpr_AST(next_Token);
    getNextToken();
    if (errorIn_Progress) return 0;
    return res;
}

// double -> <double value>
Expr_AST*
parseFltExpr(void)
{
    if (option_Debug) std::cout << "parsing a flt...\n";

    Expr_AST* res = new FltExpr_AST(next_Token);
    getNextToken();
    if (errorIn_Progress) return 0;
    return res;
}

std::vector<Expr_AST*>* parseDims(void);

IdExpr_AST*
parseArrayIdExpr(ArrayVarDecl_AST* Base)
{
    int num_Dims = Base->numDims();
    std::vector<Expr_AST*>* dims_V = parseDims();
    if (errorIn_Progress)
	return 0;
    // cast to suppress warning of signed/unsigned comparison
    else if ( (static_cast<int>(dims_V->size()) != num_Dims) ){
	std::ostringstream tmp_Stream;
	tmp_Stream << "access dimensions (" << dims_V->size();
	tmp_Stream << ") don't match the declaration of " << Base->Name();
	parseError(Base->Name(), tmp_Stream.str());
	errorIn_Progress = 1;
	return 0;
    }

    // validity check, and pre-processing for all-integer dims arrays
    int all_Ints = 1;
    std::vector<Expr_AST*>::const_iterator iter;
    for ( iter = dims_V->begin(); iter != dims_V->end(); iter++){
	if ( (tok_intV != ((*iter)->Op()).Tok() ) ) 
	    all_Ints = 0;
    }

    // As for a compile-time bound check both vectors need to be of full 
    // integer type, process dims_Final_ first when possible
    std::vector<std::string>* dims_Final = new std::vector<std::string>;
    dims_Final->reserve(num_Dims);
    if (all_Ints){
	std::vector<Expr_AST*>::const_iterator iter;
	for ( iter = dims_V->begin(); iter != dims_V->end(); iter++ )
	    dims_Final->push_back( (*iter)->Addr() );
    }

    // Now perform bound checks, when possible (return error and leave)
    // Recall that arrays are 0-based (0 - (len-1) valid access)
    int offset_V = tok_null;
    if ( (all_Ints) && (Base->allInts()) ){
	// collect info for offset
	offset_V = 0;
	int size_Parents = 1;
	int width = (Base->Expr())->TypeW();
 
	for ( int i = (num_Dims - 1); i >= 0; i-- ){
	    int tmp_Bound = atoi( (*(Base->DimsFinal()))[i].c_str() );
	    std::string tmp_ValueStr = (*dims_Final)[i];
	    int tmp_Value = atoi( tmp_ValueStr.c_str() );
	    if ( (0 > tmp_Value) || (tmp_Bound <= tmp_Value) ){
		parseError(tmp_ValueStr, "index out of array bounds");
		errorIn_Progress = 1;
		return 0;
	    }

	    offset_V += tmp_Value * size_Parents * width;
	    size_Parents *= tmp_Bound;
	}
    }
    // Get offset: Calculation example
    // Basics:      width(integer) = 8; 0-based array; result ret = 0 to start
    // Declaration: int a[5][10][20]
    // Access:      a[3][4][9]
    //     size_Parents(sp)           added to ret
    //          1                    9*sp*8 = 9*1*8         (1)
    //        1 * 20               4*sp*8 = 4*(1*20)*8      (2)
    //      1 * 20 * 10           3*sp*8 = 3*(1*20*10)*8    (3)
    // Result: ret = (1) + (2) + (3)

    token type = (Base->Expr())->Type();
    token op = (Base->Expr())->Op();
    std::string off;
    if ( (tok_null == offset_V) )
	off = "";
    else{
	std::ostringstream tmp_Stream;
	tmp_Stream << offset_V;
	off = tmp_Stream.str();
    }
    ArrayIdExpr_AST* ret;
    ret = new ArrayIdExpr_AST(type, op, all_Ints, dims_V, dims_Final,Base, off);
    if (all_Ints){
	std::ostringstream tmp_Stream;
	tmp_Stream << "(-$" <<  offset_V << ")" << Base->Addr();
	ret->setAddr(tmp_Stream.str());
    }

    return ret;
}

// id -> alpha* [alphanum | _]*
// Used for *reading an ID*, not its definition
// Disambiguates shadowed names; returning the currently active one.
IdExpr_AST*
parseIdExpr(std::string Name, int hasPrefix)
{
    if (option_Debug) std::cout << "parsing (retrieving) an Id...\n";

    std::string e_Msg1 = "array index not specified - illegal access";
    std::string e_Msg2 = "attempt to access non-array as an array";
    std::string e_Msg3 = "attempt to post-fix adjust an identifier with prefix";
    IdExpr_AST* pId;
    Decl_AST* pVD;

    if ( ( 0 == (pVD = (findVarByName(top_Env, Name))) ) ){
	varAccessError(next_Token.Lex(), 0);
	errorIn_Progress = 1;
	return 0;
    }

    if ( (0 == match(1, tok_rdopen, 0)) ){
	if (errorIn_Progress) return 0;
	; // ** TO DO: this can catch functions
    }

    // postfix ++
    else if ( (0 == match(0, tok_dplus, 0)) ){
	if (hasPrefix){
	    parseError(pVD->Addr(), e_Msg3);
	    errorIn_Progress = 1;
	    return 0;
	}
	// ** TO DO: monitor - need special check for array/function?
	IdExpr_AST* name = pVD->Expr(); // Decl::Expr() member is of right type
	pId = new IncrIdExpr_AST(name, "post", 1);
	idModInsert(postfix_Table, name, 1); // no link to the *new* object,
	getNextToken();                      // oviously, in postfix_Table
    }

    // postfix --
    else if ( (0 == match(0, tok_dminus, 0)) ){
	if (hasPrefix){
	    parseError(pVD->Addr(), e_Msg3);
	    errorIn_Progress = 1;
	    return 0;
	}
	// ** TO DO: monitor - need special check for array/function?
	IdExpr_AST* name = pVD->Expr(); // Decl::Expr() member is of right type
	pId = new IncrIdExpr_AST(name, "post", -1);
	idModInsert(postfix_Table, name, -1); // no link to the *new* object,
	getNextToken();                      // oviously, in postfix_Table
    }

    // array
    else if ( (0 == match(0, tok_sqopen, 0)) ){
	if ( !(dynamic_cast<ArrayVarDecl_AST*>(pVD)) ){
	    parseError(pVD->Addr(), e_Msg2);
	    errorIn_Progress = 1;
	    pId = 0;
	}
	else
	    pId = parseArrayIdExpr(dynamic_cast<ArrayVarDecl_AST*>(pVD));
    }

    // basic type
    else{ // ** TO DO: monitor for changes after functions added
	if (dynamic_cast<ArrayVarDecl_AST*>(pVD)){
	    parseError(pVD->Addr(), e_Msg1);
	    errorIn_Progress = 1;
	    pId = 0;
	}
	else
	    pId = pVD->Expr();
    }

    if (errorIn_Progress) return 0;
    return pId;
}

/***********************************************
* Expression parsers (+ primary switchboard)
***********************************************/
// Note: if LHS is a[i] (ArrayIdExpr), we coerce to its type when types differ.
int // 0: no coercion; 1: coerced LHS; 2: coerced RHS
checkForCoercion(Expr_AST* LHS, Expr_AST* RHS)
{
    if ( (LHS->TypeP() == RHS->TypeP()) )
	return 0;
    else if ( dynamic_cast<ArrayIdExpr_AST*>(LHS) ) // keep as separate case
	return 2;                                   // for clarity
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

// For arrays, no initialization check. For clarity, make an extra 'if' case
// which: 0 - both; 1 - LHS only; 2 - RHS only
// ** TO DO: monitor changes after adding funcitons and classes
void
checkInitialized(Expr_AST* LHS, Expr_AST* RHS)
{
    IdExpr_AST* pId;
    // initialization information for IncrIdExpr_AST is in its parent
    if (dynamic_cast<IncrIdExpr_AST*>(LHS))
	LHS = dynamic_cast<IdExpr_AST*>(LHS->Parent());
    if (dynamic_cast<IncrIdExpr_AST*>(RHS))
	RHS = dynamic_cast<IdExpr_AST*>(RHS->Parent());

    if ( !(dynamic_cast<ArrayIdExpr_AST*>(LHS)) ){
	    if ( (0 != LHS) && (pId = dynamic_cast<IdExpr_AST*>(LHS)) && 
		 !(pId->WarningEmitted()) && !(pId->isInitialized()) ){
		pId->Warned();
		parseWarning(pId->Addr(), "variable used un-initialized");
	    }
	}
    if ( !(dynamic_cast<ArrayIdExpr_AST*>(RHS)) ){
	if ( (0 != RHS) && (pId = dynamic_cast<IdExpr_AST*>(RHS)) && 
	     (pId) && !(pId->WarningEmitted()) && !(pId->isInitialized()) ){
	    pId->Warned();
	    parseWarning(pId->Addr(), "variable used un-initialized");
	}
    }
}

int
isModAssign(token T)
{
    tokenType tt = T.Tok();
    if ( (tok_assign_plus == tt) || (tok_assign_minus == tt) || 
	 (tok_assign_mult == tt) || (tok_assign_div == tt) )
	return 1;
    else
	return 0;
}

Expr_AST* parsePrimaryExpr(void);
int logOp_Tot = 0;

// Dijkstra shunting algorithm
// Example to illustrate variable usage: LHS + b * c - d
//         First time through:            After recursing when prec_2 < prec_3
//         prec_1: precedence of LHS      prec_2 + 1
//         prec_2: precedence of '+'      precedence of '*' (changed role)
//         prec_3: precedence of '*'      precedence of '-'
// The way we track recursion, could go bad for deeply recursive infix (> 99).
// Restriction should be added to a full description of the compiler/language.
// Allowing for chained assignments:
// 2nd operator's precedence (prec_0) is handed on to a recursive call of
// parseInfixRHS(), then compared to the precedence of the newly read operator.
Expr_AST* 
parseInfixRHS(int prec_1, int prec_0, Expr_AST* LHS)
{ 
    if (option_Debug) std::cout << "entering parseInfixRHS...\n";

    std::string const err_Msg = "expected primary expression";
    std::string const err_Msg2 = "illegal chaining of logical operators";
    std::string const err_Msg3 = "illegal assignment: lvalue expected";
    for (;;){
	int prec_2;
	int mod_Ass = isModAssign(next_Token);
	int go_Right = 0;
	if ( !mod_Ass ){
	    prec_2 = opPriority(next_Token.Tok());
	    go_Right = (prec_2 == prec_0) && (tok_eq == next_Token.Tok());
	}
	else{
	    prec_2 = opPriority(tok_eq); 
	    go_Right = (prec_2 == prec_0);
	}
 
	if (option_Debug)
	    std::cout << "current token (1) = " << next_Token.Lex() << "\n";

	if ( (prec_2 < prec_1) && !go_Right )
	    return LHS;
	// store it for later op creation
	token binOp1 = (isModAssign(next_Token))?token(tok_eq):next_Token; 

	Expr_AST* RHS;
	if ( !mod_Ass ){
	    getNextToken();
	    if (errorIn_Progress) return 0;
	    RHS = parsePrimaryExpr();
	    if ( (!RHS) ){
		parseError(next_Token.Lex(), err_Msg);
		errorIn_Progress = 1;
		return 0;
	    }
	}
	else{
	    RHS = LHS;
	    switch(next_Token.Tok()){
	    case tok_assign_plus:
		next_Token = token(tok_plus);
		break;
	    case tok_assign_minus:
		next_Token = token(tok_minus);
		break;
	    case tok_assign_mult:
		next_Token = token(tok_mult);
		break;
	    case tok_assign_div:
		next_Token = token(tok_div);
		break;
	    default:
		errExit(0, "illegal use of function parseInfixRHS()");
	    }
	}

	int prec_3 = opPriority(next_Token.Tok());
	if (option_Debug)
	    std::cout << "current token (2) = " << next_Token.Lex() << "\n";

	// Note: as we next read a Primary_Expr (which dispatches '-', 
	//       '!'), this handles a prefix expr following fine.
	token t = next_Token;
	mod_Ass = (tok_eq == t.Tok()) || isModAssign(t); // mod or '='
	go_Right = (prec_2 == prec_3) && mod_Ass;
	if ( (prec_2 < prec_3) || go_Right){ // flip from l-r, to r-l, until
	    RHS = parseInfixRHS(prec_2 + 1, prec_2, RHS); // reverted
	    if ( (!RHS) ){           
		parseError(next_Token.Lex(), err_Msg);
		errorIn_Progress = 1;
		return 0;
	    }
	}

	if (option_Debug) std::cout << "binOp1.Lex() = "<<binOp1.Lex() << "\n";
	int is_Assign = (tok_eq == binOp1.Tok())?1:0;
	if ( (is_Assign) && ( (dynamic_cast<IncrIdExpr_AST*>(LHS)) ||
			      !(dynamic_cast<IdExpr_AST*>(LHS))  ||
			      (0 == RHS) || !(dynamic_cast<Expr_AST*>(RHS)) ) ){
	    if ( (dynamic_cast<IdExpr_AST*>(LHS) || 
		  dynamic_cast<IncrIdExpr_AST*>(LHS)) )
		parseError(LHS->Addr(), err_Msg3);
	    else if ( !(0 == RHS) )
		parseError(RHS->Addr(), err_Msg3);
	    else
		parseError(next_Token.Lex(), err_Msg3);
	    errorIn_Progress = 1;
	    return 0;
	}

	int tmp = checkForCoercion(LHS, RHS);
	if ( (1 == tmp) && !(is_Assign) ){
	    if (option_Debug) std::cout << "coercing LHS...\n";
	    LHS = parseCoercion(LHS, RHS->Type().Tok());
	}
	else if ( (2 == tmp) || ( (1 == tmp) && (is_Assign) ) ){
	    if (option_Debug) std::cout << "coercing RHS...\n";
	    RHS = parseCoercion(RHS, LHS->Type().Tok());
	} 

	switch(binOp1.Tok()){
	case tok_plus: case tok_minus: case tok_div: case tok_mult:
	case tok_mod: 
	    checkInitialized(LHS, RHS);
	    LHS = new ArithmExpr_AST(binOp1, LHS, RHS);
	    break;
	case tok_eq: // validity check above
	    checkInitialized(0, RHS);
	    LHS = new AssignExpr_AST(dynamic_cast<IdExpr_AST*>(LHS), RHS);
	    break;
	case tok_log_or: 
	    checkInitialized(LHS, RHS);
	    LHS = new OrExpr_AST(LHS, RHS);
	    break;
	case tok_log_and:
	    checkInitialized(LHS, RHS);
	    LHS = new AndExpr_AST(LHS, RHS);
	    break;
	case tok_log_eq: case tok_log_ne: case tok_lt:
	case tok_le: case tok_gt: case tok_ge:
	    if ( (1 < ++logOp_Tot) ){
		parseError(binOp1.Lex(), err_Msg2);
		errorIn_Progress = 1;
		return 0;
	    }
	    checkInitialized(LHS, RHS);
	    LHS = new RelExpr_AST(binOp1, LHS, RHS);
	    break;
	default:
	    parseError(binOp1.Lex(), "illegal in context");
	    errorIn_Progress = 1;
	    return 0;
	}
    }
}

// After 1 infix or prefix expression has been read, dispatch an infix list,
// if appropriate.
Expr_AST*
parseInfixList(Expr_AST* LHS)
{
    if (option_Debug) std::cout << "parsing an InfixList...\n";

    Expr_AST* ret;
    if ( (tok_parclosed == next_Token.Tok()) || (tok_semi == next_Token.Tok()) )
	ret = LHS;
    else
	ret = parseInfixRHS(0, 0, LHS);

    if (errorIn_Progress) return 0;
    if ( (0 == ret) )
	ret = new NOP_AST();

    return ret;
}

// Handle the LHS (which is not a prefix expr) of a possible expr-list
Expr_AST* 
parseExpr(void)
{
    if (option_Debug) std::cout << "parsing an expr...\n";

    Expr_AST* LHS = parsePrimaryExpr();
    if (errorIn_Progress) return 0;
    if ( !LHS )
	return new NOP_AST(); // handled by caller

    return LHS;
}

Expr_AST* dispatchPrefixExpr(token t);

// entry point (head) of expression parsing
// expr -> prim op expr | -expr | !expr | prim | inc | dec | epsilon
// op -> +, -, *, /, %, ||, &&, op1
// op1 -> ==, !=, <, <=, >, >= 
// inc -> [++ | --] IdExpr
// dec -> IdExpr [++ | --]
// logOp_Tot: helps tracking that only 1 op1 type is valid in each expr 
// Function proper handles only non-prefix expressions
Expr_AST* 
dispatchExpr(void)
{
    if (option_Debug) std::cout << "dispatching an expression...\n";

    logOp_Tot = 0;
    Expr_AST* ret;
    switch(next_Token.Tok()){
    case '!': case '-': 
	ret = dispatchPrefixExpr(next_Token);
	if ( (0 == ret) && !(errorIn_Progress) )
	    ret = new NOP_AST();
	break;
    default:
	ret = parseExpr();
	if ( (0 == ret) && !(errorIn_Progress) )
	    ret = new NOP_AST();
	break;
    }

    if (errorIn_Progress) ret = 0;
    else ret = parseInfixList(ret);

    if (option_Debug)
	printIncTables();
    // adjust **only top level** expression by prefix and postfix found
    if ( !(prefix_Table.empty()) ){
	ret->setPrefix(prefix_Table);
	prefix_Table.clear();
    }
    if ( !(postfix_Table.empty()) ){ // both tables can have entries
	ret->setPostfix(postfix_Table);
	postfix_Table.clear();
    }

    return ret;
}

int
validInPrefix(token t)
{
    switch(t.Tok()){
    case tok_rdopen: case tok_minus: case tok_log_not:
    case tok_ID: case tok_intV: case tok_doubleV:
	return 0;
    default:
	return -1;
    }
}

Expr_AST* parseParensExpr(void);

// ** TO DO: monitor if handled fine with pre-/post-increments (should be)
// Handle the LHS (which is a prefix) of a possible expr-list.
// Self-contained, so can be called from within infix-list parsing
// without logical errors/pollution.
Expr_AST*
dispatchPrefixExpr(token t)
{
    if (option_Debug) std::cout << "parsing a Prefixexpr...\n"; 

    Expr_AST* ret;
    std::string err_Msg = "expected infix operator or primary expression";
    // collect legal prefixes
    std::vector<token> tok_Vec;
    while ( (tok_minus == t.Tok()) || (tok_log_not == t.Tok()) ) {
	tok_Vec.push_back(token(t));
	t = getNextToken();
	if (errorIn_Progress) break;
	if ( (-1 == validInPrefix(t)) ){
	    parseError(next_Token.Lex(), err_Msg);
	    errorIn_Progress = 1;
	}
    }
    if (errorIn_Progress) return 0;

    // process legal expression types prefix(es) operate on
    switch(next_Token.Tok()){
    case tok_ID: ret = parseIdExpr(next_Token.Lex(), 0); break;
    case tok_intV: ret = parseIntExpr(); break;
    case tok_doubleV: ret = parseFltExpr(); break;
    case tok_rdopen: ret = parseParensExpr(); break;
    default:
	parseError(next_Token.Lex(), "expected primary expression");
	errorIn_Progress = 1;
	return 0;
	break;
    }

    // apply prefixes successively
    if ( !(tok_Vec.empty()) && (ret) )
	checkInitialized(ret, 0);
    std::vector<token>::const_reverse_iterator iter;
    for (iter = tok_Vec.rbegin(); iter != tok_Vec.rend(); iter++)
	ret = new UnaryArithmExpr_AST(*iter, ret);

    if ( !ret ) // if we come here, result is already error-checked
	return new NOP_AST(); // handled by caller
    return ret;
}

// (expr) -> expr
Expr_AST*
parseParensExpr(void)
{
    if (option_Debug) std::cout << "parsing a ParensExpr...\n";

    if ( (-1 == match(0, tok_rdopen, 1)) )
	errExit(0, "invalid call of function parseParensExpr()");

    int oldLogic_Status = logOp_Tot;
    logOp_Tot = 0;
    Expr_AST* E = dispatchExpr();
    if (errorIn_Progress) return 0;

    if ( (-1 == match(0, tok_rdclosed, 1)) ){
	punctError(')', 0);
	errorIn_Progress = 1;
    }
    if (errorIn_Progress) return 0;
    logOp_Tot = oldLogic_Status;

    return E;
}

// Primary -> [inc]?id | id[inc]? | intVal | fltVal | (expr) | -expr | !expr
// inc -> [++ | --]
Expr_AST*
parsePrimaryExpr(void)
{
    if (option_Debug)
	std::cout << "parsing a Primary...: " << next_Token.Lex() << "\n";

    Expr_AST* tmp; // to suppress gcc handling of declaring vars in switch
    switch(next_Token.Tok()){
    case tok_dplus: // prefix modifier (++a)
	getNextToken();
	if ( (tok_ID != next_Token.Tok()) ){
	    parseError(next_Token.Lex(), "expected identifier after ++");
	    errorIn_Progress = 1;
	    break;
	}
	tmp = parseIdExpr(next_Token.Lex(), 1); // checks for ++a++ (and such)
	if (errorIn_Progress)                   // comes back as IdExpr_AST
	    break;
	idModInsert(prefix_Table, dynamic_cast<IdExpr_AST*>(tmp), 1);
	return new IncrIdExpr_AST(dynamic_cast<IdExpr_AST*>(tmp), "pre", 1);
	break;
    case tok_dminus: // prefix modifier (--a)
	getNextToken();
	if ( (tok_ID != next_Token.Tok()) ){
	    parseError(next_Token.Lex(), "expected identifier after --");
	    errorIn_Progress = 1;
	    break;
	}
	tmp = parseIdExpr(next_Token.Lex(), 1);
	if (errorIn_Progress)
	    break;
	idModInsert(prefix_Table, dynamic_cast<IdExpr_AST*>(tmp), -1);
	return new IncrIdExpr_AST(dynamic_cast<IdExpr_AST*>(tmp), "pre", -1);
	break;
    case tok_ID: // coming here, ID should be in symbol table
	return parseIdExpr(next_Token.Lex(), 0);
	break;
    case tok_intV: return parseIntExpr(); break;
    case tok_doubleV: return parseFltExpr(); break;
    case '(': return parseParensExpr(); break;
    case '-':
    case '!': 
	return dispatchPrefixExpr(next_Token);
	break;
    case ';': // might terminate an empty expression; don't forward
	return 0; // caller must handle properly (warning emitted elsewhere)
	break;
    default: // reporting handled by caller 
	// parseError(next_Token.Lex(), "expected primary expression");
	errorIn_Progress = 1; // fall through for return
	break;
    }
    return 0;
}

// List of the (init; cond; res) tuple of args to a For_AST
IterExprList_AST*
parseIterExprList(void)
{
    if (option_Debug) std::cout << "parsing an IterExprList...\n";

    if ( (-1 == match(0, tok_rdopen, 1)) )
	errExit(0, "invalid call of function parseIterExprList()");
    std::string err_Msg = "unexpected token while processing expression list";

    Expr_AST* e1 = dispatchExpr();
    if (errorIn_Progress) return 0;
    if ( (-1 == match(0, tok_semi, 1)) ){
	parseError(next_Token.Lex(), err_Msg);
	errorIn_Progress = 1;
	return 0;
    }

    Expr_AST* e2 = dispatchExpr();
    if (errorIn_Progress) return 0;
    if ( (-1 == match(0, tok_semi, 1)) ){
	parseError(next_Token.Lex(), err_Msg);
	errorIn_Progress = 1;
	return 0;
    }

    Expr_AST* e3;
    if ( (tok_rdclosed == next_Token.Tok()) )
	e3 = 0;
    else
	e3 = dispatchExpr();
    if (errorIn_Progress) return 0;

    if ( (-1 == match(0, tok_rdclosed, 1)) ){
	punctError(')', 0);
	errorIn_Progress = 1;
	return 0;
    }

    return new IterExprList_AST(e1, e2, e3);
}

/*********************************
*  Statement parsers
*********************************/
// process dimensions of an array
std::vector<Expr_AST*>*
parseDims(void)
{
    if ( (-1 == match(0, tok_sqopen, 0)) )
	errExit(0, "invalid use of parseDims() (should point at [)");

    std::vector<Expr_AST*>* dims = new std::vector<Expr_AST*>;
    while ( (0 == match(0, tok_sqopen, 1)) ){
	if ( (0 == match(0, tok_sqclosed, 0)) ){
	    parseError(next_Token.Lex(), "array dimension not specified");
	    errorIn_Progress = 1;
	    return 0;
	}

	Expr_AST* e = dispatchExpr();
	if (errorIn_Progress){
	    // parseError(next_Token.Lex(), "illegal array dimension");
	    // commented out to avoid double-reporting of an error
	    return 0;
	}

	tokenType t = (e->Type()).Tok();
	if ( (tok_int != t) && (tok_intV != t) ){
	    parseError(e->Addr(), "array dimensions must be of integer type");
	    errorIn_Progress = 1;
	    return 0;
	}
	dims->push_back(e);
	if ( (-1 == match(0, tok_sqclosed, 1)) ){
	    punctError(']', 0);
	    errorIn_Progress = 1;
	    return 0;
	}
    }

    return dims;
}

// array-decl -> type id[expr] [ [expr] ]* (all expr must be of integer type)
// Children:
// - the IdExpr_AST* embodies all non-size related information
// - dimensions handled in vector form (dims)
Decl_AST*
parseArrayVarDecl(IdExpr_AST* Name)
{
    Decl_AST* ret;
    std::vector<Expr_AST*>* dim_V = parseDims();
    if (errorIn_Progress)
	return 0;

    // validity check, and pre-processing for all-integer dims arrays
    int all_IntVals = 1;
    std::vector<Expr_AST*>::const_iterator iter;
    for ( iter = dim_V->begin(); iter != dim_V->end(); iter++){
	if ( (tok_intV != ((*iter)->Op()).Tok() ) ) 
	    all_IntVals = 0;
    }

    // Allocate and "> 0" check at compile-time, when possible.
    // Note As we check for bound violations only after building the entire
    // dimension vector, error recovery will step over the next instruction
    int width = Name->TypeW();
    if (all_IntVals){
	std::string this_DimStr;
	int this_Dim;
	for ( iter = dim_V->begin(); iter != dim_V->end(); iter++){
	    this_DimStr = ((*iter)->Op()).Lex();
	    if ( 0 >= (this_Dim = atoi( this_DimStr.c_str() )) ){
		parseError(this_DimStr, "array dimension must be non-negative");
		errorIn_Progress = 1;
		return 0;
	    }
	    else
		width *= atoi(this_DimStr.c_str());
	}
    }
    else // create marker that we need run-time stack adjustment
	width = 0;

    ret = new ArrayVarDecl_AST(Name, dim_V, all_IntVals, width);
    if ( !(all_IntVals) )
	emitRtError_Section = 1;

    return ret;
}

// decl -> type id;
//         type id = expr; (currently no basic ctor - easily added)
//         arrary-decl;
// Shadowing: allowed
// invariant: - exiting, we confirm ';' (decl), or move back to Id (dec+init)
//            - entering, points at Id
// Note: if we initialize, prepare to call parseAssignStmt() after
// **TO DO: monitor if we use this function also for heap
Decl_AST* 
parseVarDecl(token Type)
{
    if (option_Debug) std::cout << "parsing a var declaration...\n";

    // access error (allow for shadowing)
    if ( (tok_ID != next_Token.Tok()) )
	errExit(0, "parseVarDecl should be called pointing at tok_id");
    Env* prior_Env = findVarFrame(top_Env, next_Token.Lex());
    if ( (prior_Env == top_Env) ){
	varAccessError(next_Token.Lex(), 1);
	errorIn_Progress = 1;
	return 0;
    }

    IdExpr_AST* new_Id;
    new_Id = new IdExpr_AST(Type, next_Token);
    token t_Id = next_Token; // to reset after '='
    getNextToken(); 
    if (errorIn_Progress) return 0;

    Decl_AST* ret;
    switch(next_Token.Tok()){
    case tok_semi: // we are done - declaration only
	ret = new VarDecl_AST(new_Id);
	getNextToken();
	if (errorIn_Progress) return 0;
	break;
    case tok_eq: // prepare to call parseAssignStmt() next
	ret = new VarDecl_AST(new_Id);
	putBack('=');
	next_Token = t_Id;
	break;
    case tok_sqopen:
	ret = parseArrayVarDecl(new_Id);
	getNextToken();
	if (errorIn_Progress) return 0;
	break;
    default: 
	std::string e_M2 = "expected \'=\', \';\', or \'[\'";	
	parseError(next_Token.Lex(), e_M2);
	errorIn_Progress = 1;
	return 0;
    }

    // record in compile-time ST (rt-allocated arrays with 0 width)
    int err_Code;
    const char e_M[50] = "cannot insert \"%s\" into symbol table (code %d)";
    if ( (0!= (err_Code = addDeclToEnv(top_Env, ret, "stack"))) )
	errExit(0, e_M , new_Id->Addr().c_str(), err_Code);

    return ret;
}

// assign -> idExpr [= Expr; | ; ] 
// invariant: - upon exit, guarantees that ';' terminates
//            - upon entry, points at id
Assign_AST*
parseAssignStmt(void)
{
    if (option_Debug) std::cout << "parsing an assignment...\n";

    IdExpr_AST* LHS = parseIdExpr(next_Token.Lex(), 0);
    if ( (0 == LHS) ){
	// varAccessError(next_Token.Lex(), 0); report at IdExpr level
	errorIn_Progress = 1;
	return 0;
    }

    Expr_AST* RHS;
    tokenType t = next_Token.Tok();
    // no assignment to a++, ++a (although, by construction, could only 
    // have parsed an a++ type when we come here)
    if ( (tok_semi != t) && (dynamic_cast<IncrIdExpr_AST*>(LHS)) ){
	parseError(LHS->Addr(), "lvalue required on LHS in assignment");
	errorIn_Progress = 1;
	return 0;
    }
	
    switch(t){
    case tok_semi: 
	// As we call parseIdExpr() (and not dispatchExpr()), the private
	// table variables of an IncrIdExpr_AST are not set.
	// For the same reason, only postfix_Table can be set now.
	if ( (postfix_Table.empty()) )
	    parseWarning("", "unused statement");
	else if (dynamic_cast<IncrIdExpr_AST*>(LHS)){ //this is the case of a++;
	    LHS->setPostfix(postfix_Table);
	    postfix_Table.clear();
	}
	RHS = 0; // ** TO DO: needs thinking (with processing in visitor.h)
	break;
    case tok_eq: 
	getNextToken();
	if (errorIn_Progress) return 0;
	RHS = dispatchExpr();
	if ( (0 == RHS) ){
	    parseError(LHS->Addr(), "invalid assignment");
	    errorIn_Progress = 1;
	    return 0;
	}
	if ( (-1 == match(0, tok_semi, 0)) ){
	    punctError(';', 0);
	    errorIn_Progress = 1;
	    return 0;
	}
	break;

    case tok_assign_plus:
    case tok_assign_minus:
    case tok_assign_mult:
    case tok_assign_div:
	getNextToken();
	if (errorIn_Progress) return 0;
	RHS = dispatchExpr();
	if ( (0 == RHS) ){
	    parseError(LHS->Addr(), "invalid assignment");
	    errorIn_Progress = 1;
	    return 0;
	}
	if ( (-1 == match(0, tok_semi, 0)) ){
	    punctError(';', 0);
	    errorIn_Progress = 1;
	    return 0;
	}

	// logically, should check for coercion. However, if we coerce
	// '1' (int) to '1' (double), the ultimate SSA entry
	// + a a 1 looks the same. So we don't do it. 
	switch(t){
	case tok_assign_plus:
	    RHS = new ArithmExpr_AST(token(tok_plus), LHS, RHS);
	    break;
	case tok_assign_minus:
	    RHS = new ArithmExpr_AST(token(tok_minus), LHS, RHS);
	    break;
	case tok_assign_mult:
	    RHS = new ArithmExpr_AST(token(tok_mult), LHS, RHS);
	    break;
	case tok_assign_div:
	    RHS = new ArithmExpr_AST(token(tok_div), LHS, RHS);
	    break;
	default:
	    break;
	}
	break;

    default:
	std::ostringstream tmp_Stream;
	tmp_Stream << "\';\', \'=\', \'+=\', \'-=\', \'*=\', or \'/=\'";
	tmp_Stream << " expected"; 
	parseError(next_Token.Lex(), tmp_Stream.str());
	errorIn_Progress = 1;
	return 0;
	break;
    }

    if ( (0 != RHS) ){
	if ( (LHS->Type().Tok() != RHS->Type().Tok()) )
	    RHS = parseCoercion(RHS, LHS->Type().Tok());
    }

    Assign_AST* ret;
    checkInitialized(0, RHS);
    ret = new Assign_AST(LHS, RHS);

    getNextToken();
    if (errorIn_Progress) return 0;

    return ret;
}

Block_AST* dispatchStmt(void);

// (if_stmt) stmt -> if (expr) [ stmt | block ] // stmt is block; for clarity
// if_stmt [ epsilon | else [ stmt | block ] | if_stmt ]
// invariant: - upon entry, points at tok_if
// Type: 0 - leading if; 1 - else if
IfType_AST* 
parseIfStmt(int Type)
{
    if (option_Debug) std::cout << "parsing an if statement...\n";

    if ( (-1 == match(0, tok_if, 1)) )
	errExit(0, "parseIfStmt should be called pointing at tok_if");

    Expr_AST* expr = parseParensExpr();
    if (errorIn_Progress) return 0;
    if ( dynamic_cast<AssignExpr_AST*>(expr) )
	parseWarning("", "'=' in if-conditional - did you mean '=='?");

    // handle [ stmt | block ]
    Block_AST* LHS = dispatchStmt();
    if (errorIn_Progress) return 0;

    // will only matter if dispatched 'stmt' was, in fact, a block
    int hasElse = 0;
    if ( (tok_else == next_Token.Tok()) )
	hasElse = 1;

    IfType_AST* pIf = new If_AST(expr, LHS, Type, hasElse);
    return pIf;
}

StmtList_AST* parseBlock(void);
Stmt_AST* parseStmt(void);
IfType_AST* parseIfType(void);

// used when parsing if, for, and while stmts
Block_AST*
dispatchStmt(void)
{
    if (option_Debug) std::cout << "entering dispatchStmt()...\n";

    Block_AST* LHS;
    if ( (0 == match(0, tok_paropen, 0)) ){
	LHS = parseBlock();
	if (errorIn_Progress) LHS = 0;
    }
    else{
	top_Env = addEnv(top_Env);
	frame_Depth++;

	LHS = parseStmt();

 	top_Env = top_Env->getPrior();
	frame_Depth--;
    }

    return LHS;
}

IfType_AST* parseIfCtd(IfType_AST*); 

IfType_AST*
parseIfType(void)
{
    if (option_Debug) std::cout << "parsing an IfType...\n";

    IfType_AST* LHS = parseIfStmt(0);
    if (errorIn_Progress) return LHS;

    if ( (0 < frame_Depth) && (tok_else == next_Token.Tok()) )
	return parseIfCtd(LHS);
    else
	return LHS;
}

// Handle nested if (expr) stmt cases, where stmt = if, but no block object
// Invariant: points to 'else' upon entry
IfType_AST* 
parseIfCtd(IfType_AST* LHS)
{
    if (option_Debug) std::cout << "entering parseIfCtd...\n";

    if ( (-1 == match(0, tok_else, 1)) )
	errExit(0, "parseIfCtd should be called pointing at tok_else");
    if (errorIn_Progress) return 0;

    IfType_AST* RHS;
    if ( (tok_if == next_Token.Tok()) ){ // else if case
	RHS = parseIfStmt(1);
	if (errorIn_Progress) RHS = 0 ;
	LHS = new IfType_AST(LHS, RHS);
	if ( !(errorIn_Progress) && (tok_else == next_Token.Tok()) ){
	    LHS = parseIfCtd(LHS);
	    if (errorIn_Progress) RHS = 0;
	}
    }
    else{ // terminating else
	Block_AST* tmp = dispatchStmt();
	if (errorIn_Progress) LHS = 0;
	if ( !(errorIn_Progress) ){
	    RHS = new Else_AST(tmp);
	    LHS = new IfType_AST(LHS, RHS);
	}
    }

    return LHS;
}

// stmt -> for (expr-list) [ stmt | block ]?
// invariant: - upon entry, points at tok_while
For_AST* 
parseForStmt(void)
{
    if (option_Debug) std::cout << "parsing a for statement...\n";

    if ( (-1 == match(0, tok_for, 1)) )
	errExit(0, "parseForStmt should be called pointing at tok_for");

    // enable break and continue
    break_Enabled++;

    // handle expr-list
    IterExprList_AST* expr = parseIterExprList();
    if (errorIn_Progress) return 0;
    if ( ( 0 != expr) && ( 0 != expr->Cond() ) ) 
	if ( dynamic_cast<AssignExpr_AST*>(expr->Cond()) )
	    parseWarning("", "'=' in for-conditional - did you mean '=='?");

    // handle [ stmt | block ]
    Block_AST* LHS = dispatchStmt();
    if (errorIn_Progress)
	return 0;

    // restore break state
    break_Enabled--;

    For_AST* pWhile = new For_AST(expr, LHS);
    return pWhile;
}


// stmt -> while (expr) [ stmt | block ]?
// invariant: - upon entry, points at tok_while
While_AST* 
parseWhileStmt(void)
{
    if (option_Debug) std::cout << "parsing a while statement...\n";

    if ( (-1 == match(0, tok_while, 1)) )
	errExit(0, "parseWhileStmt should be called pointing at tok_while");

    // enable break and continue
    break_Enabled++;

    // handle expr
    Expr_AST* expr = parseParensExpr();
    if (errorIn_Progress) return 0;
    if ( dynamic_cast<AssignExpr_AST*>(expr) )
	parseWarning("", "'=' in while-conditional - did you mean '=='?");
    IterExprList_AST* expr_List = new IterExprList_AST(0, expr, 0);

    // handle [ stmt | block ]
    Block_AST* LHS = dispatchStmt();
    if (errorIn_Progress)
	return 0;

    // restore break state
    break_Enabled--;

    While_AST* pWhile = new While_AST(expr_List, LHS);
    return pWhile;
}

Break_AST*
parseBreakStmt(void)
{
    Break_AST* ret;
    if ( (-1 == match(1, tok_semi, 0)) ){
	punctError(';', 0);
	errorIn_Progress = 1;
	return ret = 0;
    }
    else
	ret = new Break_AST();

    getNextToken();
    if (errorIn_Progress) ret = 0;

    return ret;
}

Cont_AST*
parseContStmt(void)
{
    Cont_AST* ret;
    if ( (-1 == match(1, tok_semi, 0)) ){
	punctError(';', 0);
	errorIn_Progress = 1;
	return ret = 0;
    }
    else
	ret = new Cont_AST();

    getNextToken();
    if (errorIn_Progress) ret = 0;

    return ret;
}

// stmt    -> [ varDecl | expr | if-stmt | while-stmt | epsilon ]
// invariant -> leaving, guarantees a ';' has been found where needed;
//              entering, next_Token=first token of stmt, which is
//              guaranteed to be not '{' or '}'
Stmt_AST*
parseStmt(void)
{
    if (option_Debug) std::cout << "parsing a statement...\n";

    Stmt_AST* ret;
    switch(next_Token.Tok()){
    case tok_int:
	getNextToken();
	if (errorIn_Progress) break;
	ret = parseVarDecl(token(tok_int));
	break;
    case tok_double:
	getNextToken();
	if (errorIn_Progress) break;
	ret = parseVarDecl(token(tok_double));
	break;
    case tok_ID: // note: an 'empty' expr like a++; dispatches here
	ret = parseAssignStmt();
	break;
    case tok_dplus:  // case ++a;
    case tok_dminus: //      --a;
	ret = parsePrimaryExpr();
	if (errorIn_Progress){
	    ret = 0;
	    break;
	}
	// as we don't parse through dispatchExpr (c. parseAssign())
	dynamic_cast<IncrIdExpr_AST*>(ret)->setPrefix(prefix_Table); 
	prefix_Table.clear();

	match(0, tok_semi, 1); // error caught after breaking
	break;
    case tok_while:
	ret = parseWhileStmt();
	break;
    case tok_for:
	ret = parseForStmt();
	break;
    case tok_break:
	if ( !(break_Enabled) ){
	    parseError(next_Token.Lex(), "illegal without enclosing for/while");
	    getNextToken();
//	    errorIn_Progress = 1; // to allow recovery from else followed by if
	    ret = 0;
	    break;
	}
	ret = parseBreakStmt();
	break;
    case tok_cont:
	if ( !(break_Enabled) ){
	    parseError(next_Token.Lex(), "illegal without enclosing for/while");
	    getNextToken();
//	    errorIn_Progress = 1; // see comment above
	    ret = 0;
	    break;
	}
	ret = parseContStmt();
	break;
    case tok_if:
	ret = parseIfType();
	break;
    case tok_else:
	parseError(next_Token.Lex(), "else without leading if");
	getNextToken();
//	errorIn_Progress = 1; // see comment above 
	ret = 0;
	break;
    default: // empty stmt-expr
	dispatchExpr(); // parse and discard
	if (errorIn_Progress) break;
	parseWarning("", "unused expression");

	if ( (0 != match(0, tok_semi, 1)) ){
	    punctError(';', 0);
	    errorIn_Progress = 1;
	}
	ret = new NOP_AST();
	break;
    }

    // cases to improve error handling in case of errors in nested if's
    if (errorIn_Progress){
	if ( dynamic_cast<IfType_AST*>(ret) )
	    errorResetStmt();
	else
	    return errorResetStmt();
    }
    return ret;
}

StmtList_AST* parseStmtList(void);
StmtList_AST* parseStmtListCtd(StmtList_AST*);

// block -> { [stmtList | { stmtList }]* } 
// invariants -> entering, point at '{', if any
StmtList_AST*
parseBlock(void)
{
    if (option_Debug) std::cout << "parsing a block...\n";

    StmtList_AST* pSL;
    if ( (-1 == match(0, tok_paropen, 1)) )
	errExit(0, "invalid use of parseBlock() - should point at '{'");
    if ( (0 == match(0, tok_parclosed, 0)) ){
	getNextToken();
	return new StmtList_AST(0, 0);
    }

    top_Env = addEnv(top_Env);
    frame_Depth++;
    if ( (0 == match(0, tok_paropen, 0)) )
	pSL = parseStmtListCtd(0); // handles the case of opening '{'
    else
	pSL = parseStmtList();

    if ( (0 < frame_Depth) ){ // could have been reduced in error handling
	if ( (-1 == match(0, tok_parclosed, 0)) ){
	    punctError('}', 0);
	    errorIn_Progress = 1;
	    pSL = 0;
	}
	else{
	    // for management of arrays with integer expression dimensions
	    Stmt_AST* RHS = new EOB_AST();
	    pSL = new StmtList_AST(pSL, RHS);

	    top_Env = top_Env->getPrior();
	    frame_Depth--;
	}
    }

    if ( (tok_eof != next_Token.Tok()) ) getNextToken();
    if (errorIn_Progress) return 0;

    if (option_Debug) std::cout << "parsed a block...\n"; 
    return pSL;
}

// stmtLst -> { [stmt stmtLst] } | stmt | epsilon 
// Note: errors handled on level below; so we are clean here
StmtList_AST*
parseStmtList(void)
{
    if (option_Debug) std::cout << "parsing a stmtList...\n";

    Stmt_AST* LHS = parseStmt();
    if ( (0 < frame_Depth) ) // could be less if error
	return parseStmtListCtd(LHS); // points ahead
    else
	return LHS;
}

// Invariant: - upon entry, we point onto the first token of the next stmt
//            - upon return, points to '}'
// Logic: Once we see '}', we unroll from the right. As we don't read any new
//        tokens until done, each unrolled pass through the loop back sees
//        the same terminating '}', and keeps unrolling, until 
//        return LHS links back to the caller parserStmtList().
// Note: the eternal loop logic works (only) because '}' serves as a
//       terminator that doesn't have an object creation action attached.  
StmtList_AST*
parseStmtListCtd(StmtList_AST* LHS)
{
    if (option_Debug) std::cout << "entering parseStmtListCtd...\n";

    if ( (1 > frame_Depth) || (tok_eof == next_Token.Tok()) )
	errExit(0, "missing \'}\' - symbol table corrupted");

    StmtList_AST* RHS;
    for (;;){
	switch(next_Token.Tok()){
	case '{':
	    RHS = parseBlock();
	    if (!RHS) return 0;
	    break;
	case '}':
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
