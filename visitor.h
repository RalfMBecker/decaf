/********************************************************************
* Visitor.h - Visitor handling transformation of the AST to an IR
*
* Note: abstract base class defined in ast.h to break dependency
*       cycle
*
* TO DO: add visitor class deleting the AST nodes
*
********************************************************************/

#include <vector>
#include <string>
#include <sstream>

#include "ast.h"
#include "tables.h"
#include "ir.h"

class MakeIR_Visitor: public AST_Visitor{
public:
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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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

	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;

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
	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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
	std::vector<std::string> labels;
	if ( ("" != if_Next_) ){
	    labels.push_back(if_Next_);
	    if_Next_ = "";
	}
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
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

	std::vector<std::string> labels;
	if ( ("" != if_Done_) ){
	    labels.push_back(if_Done_);
	    if_Done_ = "";
	}

	std::string target = V->LChild()->Addr();

	token Op = token(tok_eq);
	std::string LHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, "", Frame);
	insertLine(line);
    }

    // **TO DO: currently only handles easiest case: stmt, no nested scopes
    void visit(If_AST* V)
    {
	std::vector<std::string> labels;
	// label appropriately
	if ( ("" != if_Next_) ) 
	    labels.push_back(if_Next_);
	else if ( ("" != if_Done_) ) // this if is a leader if 
	    labels.push_back(if_Done_);

	// update labels
	if_Next_ = makeLabel();
	if ( ("" == if_Done_) )
	    if_Done_ = makeLabel();
	
	// make iffalse SSA entry
	token Op = token(tok_iffalse);
	std::string target = V->LChild()->Addr();
	std::string LHS = "goto";
	std::string RHS = if_Next_;
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();
	IR_Line* line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);

	std::string tmpIf_Next = if_Next_; // to suppress label printing 
	std::string tmpIf_Done = if_Done_; // in stmt(s) to follow
	if_Next_ = if_Done_ = "";
	// make stmt (block) SSA entry (entries)
  	if (dynamic_cast<Stmt_AST*>(V->RChild()))
	    V->RChild()->accept(this);
	if_Next_ = tmpIf_Next;
	if_Done_ = tmpIf_Done;

	// make goto SSA entry
	labels.clear();
	Op = token(tok_goto);
	target = if_Done_;
	LHS = RHS = "";
	line = new SSA_Entry(labels, Op, target, LHS, RHS, frame_Str);
	insertLine(line);
    }

    void visit(Else_AST* V)
    {
	return;
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
};
