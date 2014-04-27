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
    void visit(Expr_AST* V) { return; }

    // objects with address set by default ctor
    void visit(Tmp_AST* V) { return; }
    void visit(IdExpr_AST* V) { return; }
    void visit(IdArrayExpr_AST* V) { return; }
    void visit(IntExpr_AST* V) { return; }
    void visit(FltExpr_AST* V) { return; }

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
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	insertLine(new SSA_Entry(labels, Op, target, "0", RHS, Frame));
    }

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

    // Even if different objects have the same code, must implement
    // separately for each. There is certainly a way around this using
    // another indirection; but we did not pursue it.
    void visit(LogicalExpr_AST* V)
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

    void visit(OrExpr_AST* V)
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

    void visit(AndExpr_AST* V)
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

    void visit(If_AST* V)
    {
	// dispatch expr - labels handled through global active_Labels_
	V->LChild()->accept(this);

	// make iffalse SSA entry
	makeLabelsIf( !(V->isElseIf()) , V->hasElse());

	label_Vec labels;
	token Op = token(tok_iffalse);
	std::string target = V->LChild()->Addr();
	std::string LHS = "goto";
	std::string RHS = if_Next_;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	Label_State* pLabels = new Label_State(frame_Str, if_Next_, if_Done_);
	// make stmt (block) SSA entry (entries), if there is at least one
	if ( (0 != V->RChild()) ){ 
	    V->RChild()->accept(this);
	    // Unhandled labels remaining; happens when, at end of inner scope,
	    //               if [- else if]* [else if | end] 
	    // and no {} around this IfType_AST (c., e.g., decaf_b5.dec).
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

    void visit(IfList_AST* V) { return; }

    // c. If_AST* visitor for logic
    void visit(Else_AST* V)
    {
	// make stmt (block) SSA entry (entries)
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	Label_State* pLabels = new Label_State(frame_Str, if_Next_, if_Done_);
	V->LChild()->accept(this);
	pLabels->Restore();

	// Ensure remaining labels are printed in next line (always after 
	// else type). ** TO DO: revisit when more labels are produced
	if ( (V->isEOB()) ){
	    label_Vec labels = pLabels->getLabels();
	    token Op = token(tok_nop);
	    std::string tmp = "";
	    IR_Line* line = new SSA_Entry(labels, Op, tmp, tmp, tmp, frame_Str);
	    insertLine(line);
	}
	else 
	    active_Labels_.push_back(if_Done_);

	if_Next_ = if_Done_ = "";
    }

    void makeLabelsIf(int Is_LeadingIf, int Has_Else)
    {
	if_Next_ = makeLabel();
	if (Has_Else && Is_LeadingIf)
	    if_Done_ = makeLabel();
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

private:
// making use-vars all static allows us to re-use across visitor calls
static int count_Tmp_;
static int count_Lab_;

static std::string if_Next_;
static std::string if_Done_;
static label_Vec active_Labels_;
};

#endif
