/********************************************************************
* Visitor.h - Visitor handling transformation of the AST to an IR
*
* Note: abstract base class defined in ast.h to break dependency
*       cycle
*
********************************************************************/

#include <string>
#include <sstream>

#include "ast.h"
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
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(CoercedExpr_AST* V)
    {
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

	insertLine(new SSA_Entry(Op, target, LHS, to_Str, Frame));
    }

    void visit(UnaryArithmExpr_AST* V)
    {
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	insertLine(new SSA_Entry(Op, target, "0", RHS, Frame));
    }

    // Even if different objects have the same code, must implement
    // separately for each. There is certainly a way around this using
    // another indirection; but we did not pursue it.
    void visit(LogicalExpr_AST* V)
    {
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(OrExpr_AST* V)
    {
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(AndExpr_AST* V)
    {
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(RelExpr_AST* V)
    {
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string RHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(Op, target, LHS, RHS, Frame);
	insertLine(line);
    }

    void visit(NotExpr_AST* V)
    {
	std::string target = makeTmp();
	V->setAddr(target);

	token Op = V->Op();
	std::string LHS = V->LChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	insertLine(new SSA_Entry(Op, target, LHS, "", Frame));
    }

    // Statements begin

    void visit(Block_AST* V) { return; }
    void visit(Stmt_AST* V) { return; } 

    void visit(VarDecl_AST* V)
    {
	std::string target = V->LChild()->Addr();
//	V->setAddr(target); // **TO DO: [debug]

	token Op = token(tok_eq);
	std::string LHS = "";
	if ( (0 != V->RChild()) )
	    std::string LHS = V->RChild()->Addr();
	Env* pFrame = V->getEnv();
	std::string frame_Str = pFrame->getTableName();

	std::cout << "\t\t\t\t\t[debug] LHS = " << LHS << "\n";

	IR_Line* line = new SSA_Entry(Op, target, LHS, "", frame_Str);
	insertLine(line);
    }

    void visit(Assign_AST* V)
    {
	// empty assignment?
	if ( (0 == V->RChild()) )
	    return;

	std::string target = V->LChild()->Addr();
	// **TO DO: [debug] monitor addr

	token Op = token(tok_eq);
	std::string LHS = V->RChild()->Addr();
	std::string Frame = V->getEnv()->getTableName();

	IR_Line* line = new SSA_Entry(Op, target, LHS, "", Frame);
	insertLine(line);
    }

    std::string makeTmp(void)
    {
	std::ostringstream tmp_Stream;
	tmp_Stream << "t" << ++count_Tmp_;
	return tmp_Stream.str();
    }

    std::string makeLab(void)
    {
	std::ostringstream tmp_Stream;
	tmp_Stream << "L" << ++count_Lab_;
	return tmp_Stream.str();
    }

private:
static int count_Tmp_;
static int count_Lab_;
};
