/********************************************************************
* Visitor.h - Visitor handling transformation of the AST to an IR
*
* Note: abstract base class defined in ast.h to break dependency
*       cycle
*
* Philosophy: At the expense of emitting an occasional NOP, this was
*             re-factored to greatly simplify the emission of labels
*             in || and && objects; and to a lesser extent in if -
*             else if - else tropes.
*
* Errors: 
* Compile-time: when visitor operates, handed on code should be 
*               compile-time error-free (handled in parsing).
* Run-time: handled here. Currently:
*                         - array bounds
*
********************************************************************/

#ifndef VISITOR_H_
#define VISITOR_H_

#include <vector>
#include <string>
#include <sstream>

#include "lexer.h"
#include "ast.h"
#include "tables.h"
#include "ir.h"

typedef std::vector<std::string> label_Vec;
extern int option_Debug;

class MakeIR_Visitor: public AST_Visitor{
public:
    // address-less objects
    void visit(Node_AST* V) 
    {
	if (option_Debug) std::cout << "visiting Node_AST...\n";
    }

    void visit(Expr_AST* V)
    {
	if (option_Debug) std::cout << "visiting Expr_AST...\n";
    }

    // objects with address set by default ctor
    void visit(Tmp_AST* V) 
    {
	if (option_Debug) std::cout << "\tvisiting TMP_AST...\n";

	if (needs_Label_){
	    insertNOP(active_Labels_, V->getEnv()->getTableName());
	    active_Labels_.clear();
	}
	needs_Label_ = 0;
    }

    void visit(IntExpr_AST* V) 
    {
	if (option_Debug) std::cout << "\tvisiting IntExpr_AST...\n";

	if (needs_Label_){
	    insertNOP(active_Labels_, V->getEnv()->getTableName());
	    active_Labels_.clear();
	}
	needs_Label_ = 0;
    }
    void visit(FltExpr_AST* V) 
    {
	if (option_Debug) std::cout << "\tvisiting FltExpr_AST...\n";

	if (needs_Label_){
	    insertNOP(active_Labels_, V->getEnv()->getTableName());
	    active_Labels_.clear();
	}
	needs_Label_ = 0;
    }

    void visit(IdExpr_AST* V) 
    {
	if (option_Debug) std::cout << "\tvisiting IdExpr_AST...\n";

	if (needs_Label_){
	    insertNOP(active_Labels_, V->getEnv()->getTableName());
	    active_Labels_.clear();
	}
	needs_Label_ = 0;
    }

    // ** TO DO
    void visit(IdArrayExpr_AST*) { return; }

    void visit(NOP_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting NOP_AST...\n";

	needs_Label_ = 0; 
	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string frame_Str;
	if ( (0 == V) ) // only when dispatched from visitor if
	    frame_Str = "";
	else
	    frame_Str = V->Addr();

	insertNOP(labels, frame_Str);
    }

    // objects needing addr update
    void visit(ArithmExpr_AST* V)
    {  
	if (option_Debug) std::cout << "\tvisiting ArithmExpr_AST...\n";

	needs_Label_ = 0;
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting ArithmExpr_AST");
	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting ArithmExpr_AST");

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);
    }

    void visit(CoercedExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting CoercedExpr_AST...\n";

	needs_Label_ = 0;
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting CoercedExpr_AST");
	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting CoercedExpr_AST");

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = token(tok_cast);
	std::string LHS = V->RChild()->Addr();  // lChild has tmp assigned to
	std::string to_Str;
	switch (V->To()){
	case tok_int: to_Str = "int"; break;
	case tok_double: to_Str = "double"; break;
	default:
	    errExit(0, "invalid use of visit(CoercedExpr_AST*)");
	}
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target,LHS, to_Str, Frame);;
	insertLine(line, iR_List);
    }

    void visit(UnaryArithmExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting UnaryArithmExpr_AST...\n";

	needs_Label_ = 0;
	std::string e = "parsing error detected when visiting UnaryArExpr_AST";
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);
	else
	    errExit(0, e.c_str());

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line, iR_List);
    }

    // no address update needed, but kept among expression visitor types
    void visit(AssignExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting AssignExpr_AST...\n";

	needs_Label_ = 0;
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting AssignExpr_AST");
	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting AssignExpr_AST");

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = V->LChild()->Addr();
	token Op = token(tok_eq);
	std::string LHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line, iR_List);
    }

    void visit(LogicalExpr_AST* V) { return; }

    // Evaluate a sequence e1 || e2 [|| e3]* left to right.
    // As soon as any e_i evaluates to true, set addr = e_i, and jump to exit 
    // (this properly represents the truth value of the sequence). 
    // If we reach the last e_k, the whole sequence is true/false depending 
    // only on e_k; so set addr = e_k.
    // Note that tree of a sequence of || expressions looks like:
    //                     Or
    //                   Or  RC
    //                 Or  RC
    //               LC RC                => unroll first to get right labels
    void visit(OrExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting OrExpr_AST...\n";
 
	std::string cond_Res = makeTmp();
	std::string cond_End = makeLabel();

	// get to bottom left
	while ( (dynamic_cast<OrExpr_AST*>(V->LChild())) )
	    V = dynamic_cast<OrExpr_AST*>(V->LChild());

	// handle expr1...
	needs_Label_ = 1;
	V->LChild()->accept(this);

	// ...and assign its result to the status variable (cond_Res)
	label_Vec labels;
	token Op = token(tok_eq);
	std::string target = cond_Res;
	std::string LHS = V->LChild()->Addr();
	std::string RHS = "";
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS,RHS, frame_Str);
	insertLine(line, iR_List);

	doOr(V, cond_Res, cond_End);
	labels.push_back(cond_End);
	insertNOP(labels, frame_Str);

	if (option_Debug){
	    std::cout << "\tleaving visitor or, and pushing ";
	    std::cout << cond_End << "\n";
	}
    }

    // if we find OrExprList = OrExpr(LHS, OrExprList), the current
    // RHS is in position V->RChild()->LChild()
    void doOr(OrExpr_AST* V, std::string cond_Res, std::string cond_End)
    {
	std::string cond_First = makeLabel();

	// make iffalse SSA entry
	label_Vec labels;
	token Op = token(tok_iffalse);
	std::string target = cond_Res;
	std::string LHS = "goto";
	std::string RHS = cond_First;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS,RHS, frame_Str);
	insertLine(line, iR_List);

	// jump over expr 2
	Op = token(tok_goto);
	target = cond_End;
	LHS = RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line, iR_List);

	// handle expr2
	needs_Label_ = 1;
	active_Labels_.push_back(cond_First);
	V->RChild()->accept(this);
	active_Labels_.clear();

	// ...and assign its result to the status variable (cond_Res)
	Op = token(tok_eq);
	target = cond_Res;
	LHS = V->RChild()->Addr();
	RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line, iR_List);

	if ( (dynamic_cast<OrExpr_AST*>(V->Parent())) )
	    doOr(dynamic_cast<OrExpr_AST*>(V->Parent()), cond_Res, cond_End);
	else{
	    if ( ("" == V->Addr()) )
		V->setAddr(cond_Res);
	    return;
	}
    }
   
    // Compare OrExpr_AST visitor for logic
    void visit(AndExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting AndExpr_AST...\n";

	std::string cond_Res = makeTmp();
	std::string cond_End = makeLabel();

	// get to bottom left
	while ( (dynamic_cast<AndExpr_AST*>(V->LChild())) )
	    V = dynamic_cast<AndExpr_AST*>(V->LChild());

	// handle expr1...
	needs_Label_ = 1;
	V->LChild()->accept(this);

	// ...and assign its result to the status variable (cond_Res)
	label_Vec labels;
	token Op = token(tok_eq);
	std::string target = cond_Res;
	std::string LHS = V->LChild()->Addr();
	std::string RHS = "";
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, RHS,frame_Str);
	insertLine(line, iR_List);

	doAnd(V, cond_Res, cond_End);
	labels.push_back(cond_End);
	insertNOP(labels, frame_Str);

	if (option_Debug){
	    std::cout << "\tleaving visitor and, and pushing ";
	    std::cout << cond_End << "\n";
	}
    }

    // if we find OrExprList = OrExpr(LHS, OrExprList), the current
    // RHS is in position V->RChild()->LChild()
    void doAnd(AndExpr_AST* V, std::string cond_Res, std::string cond_End)
    {
	std::string cond_First = makeLabel();

	// make iftrue SSA entry
	label_Vec labels;
	token Op = token(tok_iftrue);
	std::string target = cond_Res;
	std::string LHS = "goto";
	std::string RHS = cond_First;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS,RHS, frame_Str);
	insertLine(line, iR_List);

	// jump over expr 2
	Op = token(tok_goto);
	target = cond_End;
	LHS = RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line, iR_List);

	// handle expr2
	needs_Label_ = 1;
	active_Labels_.push_back(cond_First);
	V->RChild()->accept(this);
	active_Labels_.clear();

	// ...and assign its result to the status variable (cond_Res)
	Op = token(tok_eq);
	target = cond_Res;
	LHS = V->RChild()->Addr();
	RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line, iR_List);

	if ( (dynamic_cast<AndExpr_AST*>(V->Parent())) )
	    doAnd(dynamic_cast<AndExpr_AST*>(V->Parent()), cond_Res, cond_End);
	else{
	    if ( ("" == V->Addr()) )
		V->setAddr(cond_Res);
	    return;
	}
    }

    void visit(RelExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting RelExpr_AST...\n";

	needs_Label_ = 0;
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting RelExpr_AST");
	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting RelExpr_AST");

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);
    }

    void visit(NotExpr_AST* V)
    {
	if (option_Debug) std::cout << "\tvisiting NotExpr_AST...\n";

	needs_Label_ = 0;
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);
	else
	    errExit(0, "parsing error detected when visiting NotExpr_AST");
	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line, iR_List);
    }

    // Statements begin
    void visit(Block_AST* V)
    {
	if (option_Debug) std::cout << "visiting Block_AST...\n";
    }

    void visit(StmtList_AST* V)
    {
	if (option_Debug) std::cout << "visiting StmtList_AST...\n";
    }

    void visit(Stmt_AST* V) 
    {
	if (option_Debug) std::cout << "visiting Stmt_AST...\n";
    }

    void visit(EOB_AST* V)
    {
	if (option_Debug) std::cout << "visiting EOB_AST...\n";

	Env* frame = V->getEnv();
	shrinkStackVec(frame);
    }

    void visit(VarDecl_AST* V)
    {
	if (option_Debug) std::cout << "visiting VarDecl_AST...\n";

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = V->LChild()->Addr();
	token Op = token(tok_dec);
	std::string LHS = V->Type().Lex();
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, "", frame_Str);
	insertLine(line, iR_List);
    }

    // ** TO DO: could expressions of type || && need special treatment?
    //           (as they emit labels?)
    void visit(ArrayVarDecl_AST* V)
    {
	if (option_Debug) std::cout << "visiting ArrayVarDecl_AST...\n";

	std::ostringstream tmp_Stream;
	Env* pFrame = V->getEnv();
	std::string frame = pFrame->getTableName();
	std::string target;
	token Op;
	std::string LHS;
	std::string RHS;
	SSA_Entry* line;
	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	// All dimensions = integers handled in parser; if at least one
	// dimension is an integer expression, handled here. 
	if ( !(V->allInts()) ){
	    // account for width of basic type
	    target = makeTmp();
	    Op = token(tok_eq);
	    tmp_Stream << V->Expr()->TypeW();
	    LHS = tmp_Stream.str();
	    line = new SSA_Entry(labels, Op, target, LHS, "", frame);
	    insertLine(line, iR_List);

	    // generate dimension bounds, and multiply them;
	    std::vector<Expr_AST*>::iterator iter;
	    std::vector<Expr_AST*> dims = *(V->Dims());

	    // retrieve the dimenstions, and successively multiply them
	    Op = token(tok_mult);
	    std::string l_ErrTarget = makeLabel(); // begin error processing
	    for ( iter = dims.begin(); iter != dims.end(); iter++ ){
		if ( dynamic_cast<IntExpr_AST*>(*iter) || 
		     (dynamic_cast<IdExpr_AST*>(*iter)) ){
		    // calculate bound, and store result for future reference
		    LHS = target;
		    RHS = (*iter)->Addr();
		    target = makeTmp();
		    line = new SSA_Entry(labels, Op, target, LHS, RHS, frame);
		    insertLine(line, iR_List);
		    V->addToDimsFinal(RHS); // used if we later need dims
		}
		else{
		    // calculate bound, and store result for future reference
		    (*iter)->accept(this);
		    LHS = target;
		    RHS = last_Tmp_;
		    target = makeTmp();
		    line = new SSA_Entry(labels, Op, target, LHS, RHS, frame);
		    insertLine(line, iR_List);
		    V->addToDimsFinal(RHS);
		}
		// jump and continue if (expr > 0)
		compAndJumpFalse(l_ErrTarget, token(tok_lt), "0", RHS, frame);
	    }

	    // prepare jump to runtime error by pushing lNo & variable name
	    int lNo = V->Line();
	    std::string var = V->Addr();
	    pushLnoAndVar(lNo, var, l_ErrTarget, LABEL_ZERO_BOUND, frame);

	    // make room on stack
	    growStack(pFrame, target);
	}

	// declare the array
	target = V->LChild()->Addr();
	Op = token(tok_dec);
	LHS = V->Type().Lex();
	line = new SSA_Entry(labels, Op, target, LHS, "", frame);
	insertLine(line, iR_List);

	if ( !(V->allInts()) ){
	    // link the new array to its memory location
	    Op = token(tok_lea);
	    LHS = "%esp";
	    line = new SSA_Entry(labels, Op, target, LHS, "", frame);
	    insertLine(line, iR_List);
	}
    }

    void visit(Assign_AST* V)
    {
	if (option_Debug) std::cout << "visiting Assign_AST...\n";

	// empty assignment?
	if ( (0 == V->RChild()) )
	    return;

	label_Vec labels = active_Labels_;
	active_Labels_.clear();

	std::string target = V->LChild()->Addr();
	token Op = token(tok_eq);
	std::string LHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line, iR_List);
    }

    // walk down the tree (see visitor If_AST)
    void visit(IfType_AST* V) 
    {
	if (option_Debug) std::cout << "visiting IfType_AST...\n";

	while ( (dynamic_cast<IfType_AST*>(V->LChild())) )
	    V = dynamic_cast<IfType_AST*>(V->LChild());
	V->accept(this);
    }

    // -- Handling similar to the case of || and && --
    // Note that the sub-tree of an if [else if]* [else]? sequence looks
    // as follows:
    //                      IT
    //                   IT   End 
    //                IT   ElIf
    //              If  Elif
    // (Notation:
    // IT == IfType_AST,
    // End -> [else | epsilon], ElIf -> else if, If -> if)
    // Visit will naturally start at leading if; guide it from there.
    // Note: we walk to the bottom left through visitor IfType_AST.
    void visit(If_AST* V)
    {
	if (option_Debug) std::cout << "visiting If_AST...\n";
	// dispatch expr - labels handled through global active_Labels_
	needs_Label_ = 1;
	V->LChild()->accept(this);

	// make labels
	std::string if_Next;
	std::string if_Done;
	if_Next = makeLabel();
	if (V->hasElse())
	    if_Done = makeLabel();
	else
	    if_Done = if_Next;
 
	// make iffalse SSA entry
	label_Vec labels;
	token Op = token(tok_iffalse);
	std::string target = V->LChild()->Addr();
	std::string LHS = "goto";
	std::string RHS = if_Next;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS,RHS, frame_Str);
	insertLine(line, iR_List);

	// make stmt (block) SSA entry (entries), if there is at least one
	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);

	// make goto SSA entry
	if (V->hasElse()){
	    Op = token(tok_goto);
	    target = if_Done;
	    LHS = RHS = "";
	    line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	    insertLine(line, iR_List);
	}

	// make target for iffalse (which is = if_Done, if nothing follows)
	labels.push_back(if_Next);
	insertNOP(labels, frame_Str);

	// O check for next object necessary in case of error recovery
	if ( (V->hasElse()) && (0 != V->Parent()->RChild()) ){
	    if ( dynamic_cast<If_AST*>(V->Parent()->RChild()) )
		doElseIf(dynamic_cast<If_AST*>(V->Parent()->RChild()), if_Done);
	    else
		doElse(dynamic_cast<Else_AST*>(V->Parent()->RChild()), if_Done);
	}
  
    }

    void doElseIf(If_AST* V, std::string if_Done)
    {
	if (option_Debug) std::cout << "visiting doElseIf...\n";

	// dispatch expr (no labels outside {if- else if - else} scope possible)
	V->LChild()->accept(this);

	// make labels
	std::string if_Next;
	if ( !(V->hasElse()) ) // terminating else if
	    if_Next = if_Done;
	else
	    if_Next = makeLabel();
 
	// make iffalse SSA entry
	label_Vec labels;
	token Op = token(tok_iffalse);
	std::string target = V->LChild()->Addr();
	std::string LHS = "goto";
	std::string RHS = if_Next;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS,RHS, frame_Str);
	insertLine(line, iR_List);

	// make stmt (block) SSA entry (entries), if there is at least one
	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);

	// make goto SSA entry
	if (V->hasElse()){
	    Op = token(tok_goto);
	    target = if_Done;
	    LHS = RHS = "";
	    line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	    insertLine(line, iR_List);
	}

	// make target for iffalse (which is = if_Done, if nothing follows)
	labels.push_back(if_Next);
	insertNOP(labels, frame_Str);

	// walk across the tree as indicated in comment on top of If Visitor
	// O check for next object necessary in case of error recovery
	if ( (V->hasElse()) ){ // extra checks to process error cases
	    if ( !(0 == V->Parent()->Parent()) && 
		 (0 != V->Parent()->Parent()->RChild()) &&
		 dynamic_cast<IfType_AST*>(V->Parent()->Parent()) ){
		IfType_AST* pNextPar;
		pNextPar = dynamic_cast<IfType_AST*>(V->Parent()->Parent());

		IfType_AST* pNext;
		pNext = dynamic_cast<IfType_AST*>(pNextPar->RChild());
		if ( dynamic_cast<If_AST*>(pNext) )
		    doElseIf(dynamic_cast<If_AST*>(pNext), if_Done);
		else
		    doElse(dynamic_cast<Else_AST*>(pNext), if_Done);
	    }
	}
    }
 
    void doElse(Else_AST* V, std::string if_Done)
    {
	if (option_Debug) std::cout << "visiting Else_AST...\n";

	// make stmt (block) SSA entry (entries)
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	if ( (0 != V->LChild()) )
	    V->LChild()->accept(this);

	// label final exit of (if - else if - else)
	label_Vec labels; 
	labels.push_back(if_Done);
	insertNOP(labels, frame_Str);
    }

    // Expr-List: (Init, Cond, Iter)
    // (while: (0, cond, 0) )
    void visit(For_AST* V)
    {
	if (option_Debug) std::cout << "visiting For_AST...\n";

	// establish frame for for scope, and get expr-list ready
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	// expr below could be zero in error case
	IterExprList_AST* expr = dynamic_cast<IterExprList_AST*>(V->LChild()); 

	// handle initialization expression
	if ( (0 != expr) && (0 != expr->Init()) ){
	    needs_Label_ = 1;
	    expr->Init()->accept(this);
	}

	std::string label_Top = makeLabel();
	std::string label_Out = makeLabel();
	std::string label_Iter; // as target for possible continue stmt
	if ( (0 != expr) && (0!= expr->Iter()) )
	    label_Iter = makeLabel(); 

	// Dispatch condition expr - labels handled through active_Labels_
	// Cond could be 0 (infinite loop), but we always need a label
	active_Labels_.push_back(label_Top);
	needs_Label_ = 1;
	std::string target;
	if ( (0 != expr) && (0 != expr->Cond()) ){
	    expr->Cond()->accept(this);
	    if ( ("" == expr->Cond()->Addr()) )
		target = "1";
	    else
		target = expr->Cond()->Addr();
	}
	else{
	    insertNOP(active_Labels_, frame_Str);
	    target = "1"; // dummy for forever loop if no cond
	}
	active_Labels_.clear();

	// make iffalse SSA entry
	label_Vec labels;
	token Op = token(tok_iffalse);
	std::string LHS = "goto";
	std::string RHS = label_Out;
	SSA_Entry* line = new SSA_Entry(labels, Op, target, LHS,RHS, frame_Str);
	insertLine(line, iR_List);

	// handle statement
	std::string labelBreak_Old = label_Break_;
	std::string labelCont_Old = label_Cont_;
	label_Break_ = label_Out;
	label_Cont_ = ( ("" == label_Iter) )?label_Top:label_Iter;

	if ( (0 != V->RChild()) )
	    V->RChild()->accept(this);

	label_Break_ = labelBreak_Old;
	label_Cont_ = labelCont_Old;

	// Handle iteration expression - inner functions (if, for, while)
	// emit their own NOP target if needed, so not here
	if ( (0 != expr) && (0 != expr->Iter()) ){
	    needs_Label_ = 1;
	    active_Labels_.push_back(label_Iter);
	    expr->Iter()->accept(this);
	    active_Labels_.clear();
	}

	// make goto SSA entry (continuing loop)
	Op = token(tok_goto);
	target = label_Top;
	LHS = RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line, iR_List);

	// handle target of jump out from within for logic
	labels.push_back(label_Out);
	insertNOP(labels, frame_Str);
    }

    // stmt -> break;
    void visit(Break_AST* V)
    {
	if (option_Debug) std::cout << "visiting Break_AST...\n";

	// make goto SSA entry
	label_Vec labels;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	token Op = token(tok_goto);
	std::string target = label_Break_;
	SSA_Entry* line = new SSA_Entry(labels, Op, target, "", "", frame_Str);
	insertLine(line, iR_List);
    }

    // stmt -> continue;
    void visit(Cont_AST* V)
    {
	if (option_Debug) std::cout << "visiting Cont_AST...\n";

	// make goto SSA entry
	label_Vec labels;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	token Op = token(tok_goto);
	std::string target = label_Cont_;
	SSA_Entry* line = new SSA_Entry(labels, Op, target, "", "", frame_Str);
	insertLine(line, iR_List);
    }

    void insertNOP(label_Vec const& Labels, std::string Env)
    {
	token Op = token(tok_nop);
	SSA_Entry* line = new SSA_Entry(Labels, Op, "", "", "", Env);
	insertLine(line, iR_List);
    }

    std::string makeTmp(void)
    {
	static int count = 0;

	std::ostringstream tmp_Stream;
	tmp_Stream << "t" << ++count;
	std::string ret = tmp_Stream.str();
	last_Tmp_ = ret; // for arrays with integer expression bounds
	return ret;
    }

    std::string makeLabel(void)
    {
	static int count = 0;

	std::ostringstream tmp_Stream;
	tmp_Stream << "L" << ++count;
	return tmp_Stream.str();
    }

    // Helper function for run-time stack growth (see comment to companion,
    // growStack())
    void shrinkStackVec(Env* Frame)
    {
	std::vector<std::string> V = Frame->getAdj();
	if (V.empty())
	    return;
	std::string frame_Str = Frame->getTableName();

	label_Vec labels;
	token op(tok_plus);
	std::string target = "%esp";
	std::string LHS = "%esp";
	SSA_Entry* line;

	std::vector<std::string>::const_iterator iter;
	for ( iter = V.begin(); iter != V.end(); iter++ ){
	    line = new SSA_Entry(labels, op, target, LHS, *iter, frame_Str);
	    insertLine(line, iR_List);
	}
    }

    // Helper function for run-time stack management.
    // Typically,stack is extended item by item (but shrunk by the sum of 
    // these extensions; see shrinkStackVec())
    void growStack(Env* Frame, std::string Name)
    {
	Frame->addAdj(Name);

	std::string frame_Str = Frame->getTableName();
	label_Vec labels;
	token op(tok_minus);
	std::string target = "%esp";
	std::string LHS = "%esp";
	SSA_Entry* line;
	line = new SSA_Entry(labels, op, target, LHS, Name, frame_Str);
	insertLine(line, iR_List);
    }

    // Compare two variables, and jump to 'Label' if false (currently
    // only used for run-time arrays)
    void compAndJumpFalse(std::string Label, token Op, std::string LHS, 
			  std::string RHS, std::string Frame)
    {
	std::vector<std::string> labels;
	SSA_Entry* line;

	std::string target = makeTmp();
	line = new SSA_Entry(labels, Op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);

	Op = token(tok_iffalse);
	LHS = "goto";
	RHS = Label;
	line = new SSA_Entry(labels, Op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);
    }

    // Before jumping to error section (which prints the error, and exit),
    // push identifying information to the stack (currently only for 
    // run-time array declaration).
    // goto end
    // pushl var
    // pushl lineNo
    // goto error_Section
    // end nop
    void pushLnoAndVar(int lNo, std::string Var,std::string Label_ErrTarget, 
		       std::string Label_ErrExit, std::string Frame) 
    {
	std::ostringstream tmp_Stream;
	std::vector<std::string> labels;
	token op;
	std::string target;
	std::string LHS, RHS;
	SSA_Entry* line;
	std::string label_End;

	// goto to jump to end of this function (when falling through)
	op = token(tok_goto);
	target = label_End = makeLabel();
	line = new SSA_Entry(labels, op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);
	labels.clear();

	// push variable name
	labels.push_back(Label_ErrTarget);
	op = token(tok_pushl);
	tmp_Stream << "$";
	tmp_Stream << makeDsErrVar(Var);
	target = tmp_Stream.str();
	line = new SSA_Entry(labels, op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);
	tmp_Stream.str(""); // flushing is meaningless for string objects
	labels.clear();  // as there is no natural device attached to flush to

	// push line number
	tmp_Stream << "$" << lNo;
	target = tmp_Stream.str();
	line = new SSA_Entry(labels, op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);
	tmp_Stream.str("");
	labels.clear();

	// jump to error code
	target = Label_ErrExit;
	op = token(tok_goto);
	line = new SSA_Entry(labels, op, target, LHS, RHS, Frame);
	insertLine(line, iR_List);

	// emit jump target
	labels.push_back(label_End);
	insertNOP(labels, Frame);
    }

    // In the error section, to print the name of the variable in error, 
    // push it into the Ds section so other functions can retrieve it.
    std::string makeDsErrVar(std::string Value)
    {
	static int count = 0;

	std::ostringstream tmp_Stream;
	std:: string directive = ".asciiz";
	std::string name = "Evar_";
	Ds_Object* pDs;

	tmp_Stream << name << count++;
	name = tmp_Stream.str();
	tmp_Stream.str("");

	tmp_Stream << "\"" << Value << "\"";
	Value = tmp_Stream.str();
	pDs = new Ds_Object(name, directive, Value);
	Ds_Table.push_back(pDs);

	return name;
    }

    // debugging function
    void printLabels(label_Vec labels)
    {
	std::string tmp_String;
	label_Vec::const_iterator iter;
	for (iter = labels.begin(); iter != labels.end(); iter++){
	    tmp_String += *iter;
	    tmp_String += ": ";
	}
	std::cout << tmp_String << "\n";
    }

private:

static std::string last_Tmp_; // for variable length arrays

static std::string label_Break_;
static std::string label_Cont_;

static int needs_Label_; // In case a line needs a label, this triggers
                         // emission of a NOP in lines that otherwise don't
static label_Vec active_Labels_; // has the relevant labels for needs_Label_
};

#endif
