/********************************************************************
* Visitor.h - Visitor handling transformation of the AST to an IR
*
* Note: abstract base class defined in ast.h to break dependency
*       cycle
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
// label mgmt per frame helper class: when entering new scope in any context
// that might modify labels, save state, then retrieve upon return
class Label_State{
public:
Label_State(std::string Frame, std::string If_Next="", std::string If_Done="")
    : frame_(Frame), ifNext_(If_Next), ifDone_(If_Done)
    {
	all_Labels_ = label_Vec();
	if ( ("" != If_Next) ) all_Labels_.push_back(If_Next);
	if ( ("" != If_Done) ) all_Labels_.push_back(If_Done);

	MakeIR_Visitor::if_Next_ = "";
	MakeIR_Visitor::if_Done_ = "";
    }

    label_Vec getLabels(void) const { return all_Labels_; }

    void Restore(void) const
    {
	MakeIR_Visitor::if_Next_ = ifNext_;
	MakeIR_Visitor::if_Done_ = ifDone_;
    }

private:
    std::string frame_;
    std::string ifNext_;
    std::string ifDone_;

    label_Vec all_Labels_;
};

    // address-less objects
    void visit(Node_AST* V) { return; }
    void visit(Expr_AST* V){ return; }

    // objects with address set by default ctor
    void visit(Tmp_AST* V) { return; }
    void visit(IdExpr_AST* V) { return; }
    void visit(IdArrayExpr_AST* V) { return; }
    void visit(IntExpr_AST* V) { return; }
    void visit(FltExpr_AST* V) { return; }

    void visit(NOP_AST* V)
    {
	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

	std::string frame_Str;
	if ( (0 == V) ) // only when dispatched from visitor if
	    frame_Str = " n/a";
	else{
	    Env* pFrame = V->getEnv();
	    std::string frame_Str = pFrame->getTableName();
	}

	insertNOP(labels, "n/a");
    }

    // objects needing addr update
    void visit(ArithmExpr_AST* V)
    {
	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(CoercedExpr_AST* V)
    {
	label_Vec labels;
	// ** TO DO: simplify this expression throughout
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

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

	insertLine(new SSA_Entry(labels, Op, target, LHS, to_Str, Frame));
    }

    void visit(UnaryArithmExpr_AST* V)
    {
	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	insertLine(new SSA_Entry(labels, Op, target, LHS, "", Frame));
    }

    // no address update needed, but kept among expression visitor types
    void visit(AssignExpr_AST* V)
    {
	// empty assignment?
	if ( (0 == V->RChild()) )
	    return;

	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

	std::string target = V->LChild()->Addr();
	token Op = token(tok_eq);
	std::string LHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line);
    }


    // Even if different objects have the same code, it is easiest
    // to implement them separately. 
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
	if (option_Debug) std::cout << "entering visitor or...\n";
 
	std::string cond_Res = makeTmp();
	std::string cond_End = makeLabel();

	// get to bottom left
	while ( (dynamic_cast<OrExpr_AST*>(V->LChild())) )
	    V = dynamic_cast<OrExpr_AST*>(V->LChild());

	// handle expr1...
	label_Vec labels;
	if ( !handleInitialAndOrLables(V->LChild()) ) // do we label
	    labels = active_Labels_; // during expr, or when assigning expr?
	V->LChild()->accept(this);
	active_Labels_.clear();

	// ...and assign its result to the status variable (cond_Res)
	token Op = token(tok_eq);
	std::string target = cond_Res;
	std::string LHS = V->LChild()->Addr();
	std::string RHS = "";
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	doOr(V, cond_Res, cond_End);
	active_Labels_.push_back(cond_End);

	if (option_Debug){
	    std::cout << "\tleaving visitor or, and pushing ";
	    std::cout << cond_End << "\n";
	}
    }

    // if we find OrExprList = OrExpr(LHS, OrExprList), the current
    // RHS is in position V->RChild()->LChild()
    void doOr(OrExpr_AST* V, std::string cond_Res, std::string cond_End)
    {
	if (option_Debug) std::cout << "entering visitor doOr...\n";

	std::string cond_First = makeLabel();

	// make iffalse SSA entry
	label_Vec labels;
	token Op = token(tok_iffalse);
	std::string target = cond_Res;
	std::string LHS = "goto";
	std::string RHS = cond_First;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	// jump over expr 2
	Op = token(tok_goto);
	target = cond_End;
	LHS = RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	// handle expr2
	if ( checkExprTarget(V->RChild()) || // visitors printing labels
	     dynamic_cast<AndExpr_AST*>(V->RChild()) )
	    active_Labels_.push_back(cond_First);
	else // those that don't: label assignment at end 
	    labels.push_back(cond_First); 
	V->RChild()->accept(this);

	// If we go from 'and' to 'or', or vice versa, cond_End of child
	// needs printing in the calling procedure.
	if ( dynamic_cast<AndExpr_AST*>(V->RChild()) )
	    labels = active_Labels_;
	active_Labels_.clear();

	// ...and assign its result to the status variable (cond_Res)
	Op = token(tok_eq);
	target = cond_Res;
	LHS = V->RChild()->Addr();
	RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

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
	if (option_Debug) std::cout << "entering visitor and...\n";

	std::string cond_Res = makeTmp();
	std::string cond_End = makeLabel();

	// get to bottom left
	while ( (dynamic_cast<AndExpr_AST*>(V->LChild())) )
	    V = dynamic_cast<AndExpr_AST*>(V->LChild());

	// handle expr1...
	label_Vec labels;
	if ( !handleInitialAndOrLables(V->LChild()) ) // do we label
	    labels = active_Labels_; // during expr, or when assigning expr?
	V->LChild()->accept(this);
	active_Labels_.clear();

	// ...and assign its result to the status variable (cond_Res)
	token Op = token(tok_eq);
	std::string target = cond_Res;
	std::string LHS = V->LChild()->Addr();
	std::string RHS = "";
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	doAnd(V, cond_Res, cond_End);
	active_Labels_.push_back(cond_End);

	if (option_Debug){
	    std::cout << "\tleaving visitor and, and pushing ";
	    std::cout << cond_End << "\n";
	}
    }

    // if we find OrExprList = OrExpr(LHS, OrExprList), the current
    // RHS is in position V->RChild()->LChild()
    void doAnd(AndExpr_AST* V, std::string cond_Res, std::string cond_End)
    {
	if (option_Debug) std::cout << "entering visitor doAnd...\n";

	std::string cond_First = makeLabel();

	// make iftrue SSA entry
	label_Vec labels;
	token Op = token(tok_iftrue);
	std::string target = cond_Res;
	std::string LHS = "goto";
	std::string RHS = cond_First;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	// jump over expr 2
	Op = token(tok_goto);
	target = cond_End;
	LHS = RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	// handle expr2
	if ( checkExprTarget(V->RChild()) || // visitors printing labels
	     dynamic_cast<OrExpr_AST*>(V->RChild())) 
	    active_Labels_.push_back(cond_First);
	else // those that don't: label assignment at end
	    labels.push_back(cond_First); 
	V->RChild()->accept(this);

	// If we go from 'and' to 'or', or vice versa, cond_End of child
	// needs printing in the calling procedure.
	// Note: by precedence, this case should never happen. For safety.
	if ( dynamic_cast<OrExpr_AST*>(V->RChild()) )
	    labels = active_Labels_;
	active_Labels_.clear();

	// ...and assign its result to the status variable (cond_Res)
	Op = token(tok_eq);
	target = cond_Res;
	LHS = V->RChild()->Addr();
	RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

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
	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(NotExpr_AST* V)
    {
	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}

	std::string target = makeTmp();
	V->setAddr(target);
	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	insertLine(new SSA_Entry(labels, Op, target, LHS, "", Frame));
    }

    // Statements begin
    void visit(Block_AST* V) { return; }
    void visit(StmtList_AST* V) { return; }
    void visit(Stmt_AST* V) { return; } 

    void visit(VarDecl_AST* V)
    {
	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}
	std::string target = V->LChild()->Addr();
	token Op = token(tok_dec);
	std::string LHS = V->Type().Lex();
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();

	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, "", frame_Str);
	insertLine(line);
    }

    void visit(Assign_AST* V)
    {
	// empty assignment?
	if ( (0 == V->RChild()) )
	    return;

	label_Vec labels;
	if ( !(active_Labels_.empty()) ){
	    labels = active_Labels_;
	    active_Labels_.clear();
	}
	std::string target = V->LChild()->Addr();
	token Op = token(tok_eq);
	std::string LHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line);
    }

    void visit(IfType_AST* V) { return; }

    void visit(If_AST* V)
    {
	// Dispatch expr - labels handled through global active_Labels_.
	// If the expression is "empty", put label in front of a nop SSA.
	if ( (1 == checkExprTarget(V->LChild())) )
	    V->LChild()->accept(this);
	else{
	    NOP_AST* dummy = 0;
	    visit(dummy);
	}

	makeLabelsIf( !(V->isElseIf()) , V->hasElse());

	// make iffalse SSA entry
	label_Vec labels = active_Labels_; // to catch exit points (eg, Or/And)
	token Op = token(tok_iffalse);
	std::string target = V->LChild()->Addr();
	std::string LHS = "goto";
	std::string RHS = if_Next_;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);
	active_Labels_.clear();

	Label_State* pLabels = new Label_State(frame_Str, if_Next_, if_Done_);
	// make stmt (block) SSA entry (entries), if there is at least one
	if ( (0 != V->RChild()) ){ 
	    V->RChild()->accept(this);
	    // Unhandled labels remaining; happens when, at end of inner scope,
	    //               if [- else if]* [else if | end] 
	    // and no {} around this IfType_AST (c., e.g., decaf_b5.dec - 
	    // statement triggering this is else if (a > 0) ).
	    if ( !(active_Labels_.empty()) ){
		labels = active_Labels_;
		insertNOP(labels, frame_Str);
		active_Labels_.clear();
	    }
	}
	pLabels->Restore();

	// make goto SSA entry
	if (V->hasElse()){
	    labels.clear();
	    Op = token(tok_goto);
	    target = if_Done_;
	    LHS = RHS = "";
	    line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	    insertLine(line);
	}

	// If this is not followed by a statement, we would miss printing
	// out remaining target labels. Take care of that.
	if ( (V->isEOB()) ){
	    insertNOP(pLabels->getLabels(), frame_Str);
	    if_Next_ = if_Done_ = "";
	}
	else if ( (V->hasElse()) ){
	    active_Labels_.push_back(if_Next_);
	    if_Next_ = "";
	}
	else{
	    active_Labels_ = pLabels->getLabels();
	    if_Next_ = if_Done_ = "";
	}

	delete pLabels;
    }

    // c. If_AST* visitor for logic
    void visit(Else_AST* V)
    {
	// make stmt (block) SSA entry (entries)
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	Label_State* pLabels = new Label_State(frame_Str, if_Next_, if_Done_);
	V->LChild()->accept(this); // this does not mirror treatment in if
	pLabels->Restore();        // visitor (go through logic)

	// Ensure remaining labels are printed in next line (always after 
	// else type).
	if ( (V->isEOB()) )
	    insertNOP(pLabels->getLabels(), frame_Str);
	else
	    active_Labels_ = pLabels->getLabels();

	delete pLabels;
	if_Next_ = if_Done_ = "";
    }

    void makeLabelsIf(int Is_LeadingIf, int Has_Else)
    {
	if_Next_ = makeLabel();
	if (Has_Else && Is_LeadingIf)
	    if_Done_ = makeLabel();
    }

    // helper to make sure a target line for labels is always emitted
    int checkExprTarget(Node_AST* E)
    {
	// **TO DO: evaluate later if also: (dynamic_cast<IdArrayExpr_AST*>(E))
	// expressions whose visitors produce no IR line
	if ( dynamic_cast<Tmp_AST*>(E) || dynamic_cast<IdExpr_AST*>(E) ||  
	     dynamic_cast<IntExpr_AST*>(E) || dynamic_cast<FltExpr_AST*>(E) )
	    return 0;
	else
	    return 1;
    }

    // If LChild of an 'and' or 'or' expression is one of the opposite type,
    // whether it prints a label or not depends on the child's LChild. 
    // Tree type:                Or
    //                        And
    //                      Expr (not Or, not And)
    // As the function is only applied at the deepest level or a list of Or
    // expressions (see visit(OrExpr_AST*)), we can check for a child of 
    // the same type, as this will never be the case (easier coding).
    // In a case like           Or (1)
    //                       And
    //                     Or (2)
    // only the deepeest level (2) will matter - no recursive descending needed.
    // RV: 1 - next expression will print labels; 0 - not
    // **TO DO: This should work; might want to test more.
    int handleInitialAndOrLables(Node_AST* E)
    {
	if ( !checkExprTarget(E) || // visitors !printing labels
	     ( dynamic_cast<LogicalExpr_AST*>(E) &&
	       !checkExprTarget(E->LChild()) ) )
	    return 0;
	else
	    return 1;
    }

    void insertNOP(label_Vec const& Labels, std::string Env)
    {
	token Op = token(tok_nop);
	IR_Line* line = new SSA_Entry(Labels, Op, "", "", "", Env);
	insertLine(line);
    }

    std::string makeTmp(void)
    {
	std::ostringstream tmp_Stream;
	tmp_Stream << "t" << ++count_Tmp_;
	return tmp_Stream.str();
    }

    std::string makeLabel(void)
    {
	std::ostringstream tmp_Stream;
	tmp_Stream << "L" << ++count_Lab_;
	return tmp_Stream.str();
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
// **TO DO: for now and simplicity, make all private vars static
static int count_Tmp_;
static int count_Lab_;

static std::string if_Next_;
static std::string if_Done_;

static label_Vec active_Labels_;
};

#endif
