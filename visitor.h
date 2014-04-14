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

	insertLine(new SSA_Entry(Op, target, LHS, RHS, Frame));
    }

    void visit(CoercedExpr_AST* V) { }
    void visit(UnaryArithmExpr_AST* V) { }
    void visit(LogicalExpr_AST* V) { }
    void visit(OrExpr_AST* V) { }
    void visit(AndExpr_AST* V) { }
    void visit(RelExpr_AST* V) { }
    void visit(NotExpr_AST* V) { }

    std::string makeTmp(void)
    {
	std::ostringstream tmp_Stream;
	tmp_Stream << "t" << ++count_;
	return tmp_Stream.str();
    }

    private:
	static int count_;
};
