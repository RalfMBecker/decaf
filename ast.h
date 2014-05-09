/***********************************************************************
* ast.h - AST for Decaf
*
* Dispatch of children: 
*        - for objects not needing to emit labels, the usual semantic 
*          applies (e.g., see NODE_AST)
*        - for those who might receive lables, the parent visitor
*          dispatches for its children
*
***********************************************************************/

#ifndef AST_H_
#define AST_H_

#include <string>
#include <map>
#include <sstream>
#include "lexer.h"

extern int option_Debug;

// forward declarations
extern std::map<std::string, int> typePrec_Table;
extern std::map<std::string, int> typeWidth_Table;
class Env;
extern Env* top_Env;

// forward declare AST hierarchy to resolve Visitor/AST cyclicality;
// and add Visitor abstract base class to finish untangling the pattern
class Node_AST;
class Expr_AST;
class Tmp_AST;
class IdExpr_AST;
class IdArrayExpr_AST;
class IntExpr_AST;
class FltExpr_AST;
class ArithmExpr_AST;
class CoercedExpr_AST;
class UnaryArithmExpr_AST;
class AssignExpr_AST;
class LogicalExpr_AST;
class OrExpr_AST;
class AndExpr_AST;
class RelExpr_AST;
class NotExpr_AST;

class NOP_AST;
class Block_AST;
class StmtList_AST;
class Stmt_AST;
class VarDecl_AST;
class Assign_AST;
class IfType_AST;
class If_AST;
class For_AST;
class Break_AST;
class Cont_AST;

class AST_Visitor{
public: 
    virtual void visit(Node_AST*) = 0;

    virtual void visit(Expr_AST*) = 0;
    virtual void visit(Tmp_AST*) = 0;
    virtual void visit(IdExpr_AST*) = 0;
    virtual void visit(IdArrayExpr_AST*) = 0;
    virtual void visit(IntExpr_AST*) = 0;
    virtual void visit(FltExpr_AST*) = 0;
    virtual void visit(ArithmExpr_AST*) = 0;
    virtual void visit(CoercedExpr_AST*) = 0;
    virtual void visit(UnaryArithmExpr_AST*) = 0;
    virtual void visit(AssignExpr_AST* V) = 0;
    virtual void visit(LogicalExpr_AST*) = 0;
    virtual void visit(OrExpr_AST*) = 0;
    virtual void visit(AndExpr_AST*) = 0;
    virtual void visit(RelExpr_AST*) = 0;
    virtual void visit(NotExpr_AST*) = 0;

    virtual void visit(NOP_AST*) = 0;
    virtual void visit(Block_AST*) = 0;
    virtual void visit(StmtList_AST*) = 0;
    virtual void visit(Stmt_AST*) = 0;
    virtual void visit(VarDecl_AST*) = 0;
    virtual void visit(Assign_AST*) = 0;
    virtual void visit(IfType_AST*) = 0;
    virtual void visit(If_AST*) = 0;
    virtual void visit(For_AST*) = 0;
    virtual void visit(Break_AST*) = 0;
    virtual void visit(Cont_AST*) = 0;

    ~AST_Visitor();
};

/***************************************
* Base class
***************************************/
// parents manage their children: once they are created, add a pointer
// to parent in any child
class Node_AST{
public:
Node_AST(Node_AST* lC=0, Node_AST* rC=0)
    : parent_(0), lChild_(lC), rChild_(rC), line_(line_No), col_(col_No),
	addr_(""), env_(top_Env)
    {
	if ( (0 != lChild_) )
	    lChild_->setParent(this);
	if ( (0 != rChild_) )
	    rChild_->setParent(this);
    }

    ~Node_AST() {}

    virtual int Line(void) const { return line_; }
    virtual int Col(void) const { return col_; }
    virtual Node_AST* Parent(void) const { return parent_; }
    virtual Node_AST* LChild(void) const { return lChild_; }
    virtual Node_AST* RChild(void) const { return rChild_; }
    virtual std::string Addr(void) const { return addr_; }
    virtual Env* getEnv(void) const { return env_; }

    void setParent(Node_AST* Par) { parent_ = Par; }
    virtual void setAddr(std::string Addr) { addr_ = Addr; }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }

protected:
    Node_AST* parent_;
    Node_AST* lChild_;
    Node_AST* rChild_;
    int line_;
    int col_;
    std::string addr_;
    Env* env_;
    static int label_Count_;
};

/***************************************
* Statement base classes
***************************************/
class Block_AST: public Node_AST{
public:
Block_AST(Node_AST* LHS = 0, Node_AST* RHS = 0)
    : Node_AST(LHS, RHS)
    {
	if (option_Debug) std::cout << "\tcreated a Block_AST\n";
    }

    ~Block_AST() {} 

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }
};

class StmtList_AST: public Block_AST{
public:
StmtList_AST(Node_AST* LHS = 0, Node_AST* RHS = 0)
    : Block_AST(LHS, RHS)
    {
	if (option_Debug) std::cout << "\tcreated a StmtList_AST\n";
    }

    ~StmtList_AST() {} 

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }
};

class Stmt_AST: public StmtList_AST{
public:
Stmt_AST(Node_AST* LC = 0, Node_AST* RC = 0)
    : StmtList_AST(LC, RC)
    {
	if (option_Debug) std::cout << "\tcreated a Stmt_AST\n";
    }
 
    ~Stmt_AST() {}

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }
};

/***************************************
* Expression parent class
***************************************/
class ExprList_AST: public Node_AST{
public:
ExprList_AST(Node_AST* LHS, Node_AST* RHS)
    : Node_AST(LHS, RHS)
    {
	if (option_Debug) std::cout << "\tcreated an ExprList...\n";
    }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }
};

// Arithmetic, logical, basic, and access (array) types
// Natural child of both Stmt_AST and ExprList_AST. However, multiple
// inheritance would not be logical; so we choose one, and deal with
// ugly casts if we need parenthood to the other (only once).
class Expr_AST: public ExprList_AST{
public:
Expr_AST(token Type=token(), token OpTor=token(), 
	 Node_AST* lc=0, Node_AST* rc=0)
    : ExprList_AST(lc, rc), type_(Type), op_(OpTor)
    {
	if ( ( "" != type_.Lex() ) ){ // in case of default constructor
	    typeW_ = setWidth();
	    typeP_ = setPriority();
	}
	else
	    typeW_ = typeP_ = -1;
	if (option_Debug){
	    std::cout << "\tcreated Expr with op = " << op_.Lex();
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    ~Expr_AST() {}

    virtual int setWidth(void)
    {
	if ( (typeWidth_Table.end() != typeWidth_Table.find(type_.Lex())) )
	    return typeWidth_Table[type_.Lex()];
	else
	    return -1;
    }

    virtual int setPriority(void)
    {
	if ( (typePrec_Table.end() != typePrec_Table.find(type_.Lex())) )
	    return typePrec_Table[type_.Lex()];
	else
	    return -1;
    }

    // for use in classes
    virtual void forceWidth(int w) { typeW_ = w; }
	
    token Type(void) const { return type_; }
    token Op(void) const { return op_; }
    int TypeW(void) const { return typeW_; }
    int TypeP(void) const { return typeP_; }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }

protected:
    token type_; // (tok_int, "int")
    token op_;   // (tok_plus, "+")
    int typeW_;
    int typeP_;
};

/***************************************
* Expression terminals
***************************************/
class Tmp_AST: public Expr_AST{
public:
Tmp_AST(token Type)
    : Expr_AST(Type, token(tok_tmp), 0, 0)
    {
	std::stringstream tmp;
	tmp << "t" << ++count_;
	setAddr(tmp.str());
	op_.SetTokenLex(addr_);
	if (option_Debug) std::cout << "\tcreated tmp = " << addr_ << "\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    static int count_;
};

// ***parts of a fct declaration can probably be folded into this
class IdExpr_AST: public Expr_AST{
public:
IdExpr_AST(token Type, token Op)
    : Expr_AST(Type, Op, 0, 0) 
    { 
	setAddr(Op.Lex());
	if (option_Debug) std::cout << "\tcreated an Id = " << addr_ << "\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// ***TO DO: revisit when used (in progress)***
// Cannot be coerced.
class IdArrayExpr_AST: public IdExpr_AST{
public:
    IdArrayExpr_AST(token Type, token Op, int Size)
	: IdExpr_AST(Type, Op), size_(Size)
    {
	if ( (-1 != typeW_) )
	    typeW_ *= size_;
	if (option_Debug)
	    std::cout << "\tcreated an array with Id = " << addr_  << "\n";
    }

    int Size(void) const { return size_; }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    int size_;
};

class IntExpr_AST: public Expr_AST{
public:
IntExpr_AST(token Op)
    : Expr_AST(token(tok_int), Op, 0, 0) 
    {
	setAddr(Op.Lex());
	if (option_Debug)
	    std::cout << "\tcreated IntExpr with value = " << addr_ << "\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class FltExpr_AST: public Expr_AST{
public:
FltExpr_AST(token Op)
    : Expr_AST(token(tok_double), Op, 0, 0) 
    {
	setAddr(Op.Lex()); 
	if (option_Debug)
	    std::cout << "\tcreated FltExpr with value = " << addr_ << "\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// Class is useful in case of semantically important 'empty expressions';
// e.g., in if (expr) stmt clauses, to ease emitting jump lables to expr.
// It is probably not necessary for empty statements, but sometimes emitted
// if we find one (it cannot hurt).
class NOP_AST: public Expr_AST{
public:
NOP_AST(void)
    : Expr_AST(token(tok_nop), token(tok_nop), 0, 0) 
 
    {
	if (option_Debug)
	    std::cout << "\tcreated NOP \n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// ******TO DO: STRING (etc.)********************

/***************************************
* Expression Non-logical children
***************************************/
// Upon creating this object, LHS->TypeP() == RHS->TypeP()
// Per recommendation of Effective C++, don't throw error in ctor, though.
class ArithmExpr_AST: public Expr_AST{
public:
ArithmExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), Op, LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated ArithmExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// replaces old position of LHS in AST, with new LC being the TMP
class CoercedExpr_AST: public Expr_AST{
public:
CoercedExpr_AST(Expr_AST* TMP, Expr_AST* Expr)
    : Expr_AST(token(TMP->Type()), token(tok_eof), TMP, Expr),
	from_(Expr->Type().Tok()), to_(TMP->Type().Tok())
    {
	if (option_Debug){
	    std::cout << "\tcreated CoercedExpr with type = "; 
	    std::cout << type_.Lex() << "\n";
	}
    }

    tokenType From() const { return from_; }
    tokenType To() const {return to_; }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    tokenType from_; // for easier access by visitor
    tokenType to_;
};

class UnaryArithmExpr_AST: public Expr_AST{
public:
UnaryArithmExpr_AST(token Op, Expr_AST* LHS)
    : Expr_AST(token(LHS->Type()), Op, LHS, 0)
    {
	if (option_Debug){
	    std::cout << "\tcreated Unary ArithmExpr with op = " << op_.Lex(); 
	    std::cout  << ", type = " << type_.Lex() << "\n";
	}
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// ***To DO: treat array in expr as sub class of this?***
class AssignExpr_AST: public Expr_AST{
public:
AssignExpr_AST(IdExpr_AST* Id, Expr_AST* Expr)
    : Expr_AST(token(Id->Type()), token(tok_eq), Id, Expr) 
    {
	setAddr(Id->Op().Lex());
	if (option_Debug)
	    std::cout << "\tcreated AssignExpr_AST with LHS = "<< addr_<< "\n";
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

/***************************************
* Expression Logical children
***************************************/
class LogicalExpr_AST: public Expr_AST{
public:
LogicalExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), Op, LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated LogicalExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    ~LogicalExpr_AST() {}

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// implement ||
class OrExpr_AST: public LogicalExpr_AST{
public:
OrExpr_AST(Expr_AST* LHS, Expr_AST* RHS)
    : LogicalExpr_AST(token(tok_log_or), LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated OrExpr with op = " << op_.Lex();
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// implement && (as, in ultimate code generation, we short-circuit, it 
// makes sense to track this differently from ||)
class AndExpr_AST: public LogicalExpr_AST{
public:
AndExpr_AST(Expr_AST* LHS, Expr_AST* RHS)
    : LogicalExpr_AST(token(tok_log_and), LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated AndExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// implement <, <=, >, >=, ==, !=
class RelExpr_AST: public LogicalExpr_AST{
public:
RelExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : LogicalExpr_AST(Op, LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated RelExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// implement (logical) ! (chaining not allowed)
class NotExpr_AST: public LogicalExpr_AST{
public:
NotExpr_AST(token(tok_log_not), Expr_AST* LHS)
    : LogicalExpr_AST(token(tok_log_not), LHS, 0)
    {
	if (option_Debug){
	    std::cout << "\tcreated NotExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

/***************************************
* Non-logical or procedural statements
***************************************/
class Break_AST: public Stmt_AST{
public:
Break_AST(void)
    : Stmt_AST(0, 0)
    {
	if (option_Debug) std::cout<< "\tcreated a Break_AST\n";
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class Cont_AST: public Stmt_AST{
public:
Cont_AST(void)
    : Stmt_AST(0, 0)
    {
	if (option_Debug) std::cout<< "\tcreated a Cont_AST\n";
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// ***To DO:  give array declarations their own class (?)***
class VarDecl_AST: public Stmt_AST{
public:
VarDecl_AST(IdExpr_AST* Id)
    : Stmt_AST(Id, 0), type_(Id->Type())
    {
	setAddr(Id->Op().Lex());
	if (option_Debug)
	    std::cout<< "\tcreated Decl_AST with addr = " << addr_ << "\n";
    }

    token Type(void) const { return type_; }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }
private:
    token type_;
};

class Assign_AST: public Stmt_AST{
public:
Assign_AST(IdExpr_AST* Id, Expr_AST* Expr)
    : Stmt_AST(Id, Expr)
    {
	setAddr(Id->Op().Lex());
	if (option_Debug)
	    std::cout << "\tcreated Assign_AST with LHS = " << addr_ << "\n";
    }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }
};

/***************************************
* Logical and procedural statements
***************************************/
// Wrapper function helping with the fact that if in an 'if' clause the 
// statement is itself an if clause. As we keep going on the inner scope
// level as long as we see a pattern 
//              if [else if]* [else]?
// the inner scope statement list above is treated as if it were a single
// statement (stmt) in the semantic pattern 
//              if (expr) stmt
// Using the below object, we can create a statement list, then wrap it into 
// an IfType_AST (which is a statement by inheritance) child.
// To see how it will be wrapped, compare the visitor of If_AST
class IfType_AST: public Stmt_AST{
public:
IfType_AST(Node_AST* LHS, Node_AST* RHS=0)
	: Stmt_AST(LHS, RHS) 
    {
	if (option_Debug) std::cout << "created an IfType...\n";
    }

    ~IfType_AST() {}

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class If_AST: public IfType_AST{
public:
If_AST(Expr_AST* Expr, Block_AST* Block, int ElseIf, int HasElse)
    : IfType_AST(Expr, Block), isElse_If_(ElseIf), has_Else_(HasElse)
    {
	if (option_Debug){
	    std::cout << "\tcreated If_AST, type ";
	    if (ElseIf) std::cout << "else if\n";
	    else std::cout << "leading if\n";
	}
    }

    int isElseIf(void) const { return isElse_If_; }
    int hasElse(void) const { return has_Else_; }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    int isElse_If_;
    int has_Else_;
};

// We need an Else stmt object as a wrapper around the block (or stmt) 
// the object actually embodies, for unified treatment in visitor
class Else_AST: public IfType_AST{
public:
Else_AST(Block_AST* Block)
    : IfType_AST(Block, 0)
    {
	if (option_Debug) std::cout << "\tcreated Else_AST \n";
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class For_AST: public Stmt_AST{
public:
For_AST(ExprList_AST* Expr, Block_AST* Block)
    : Stmt_AST(Expr, Block)
    {
	if (option_Debug)
	    std::cout << "\tcreated a For_AST\n";
    }

    ~For_AST() {}

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class While_AST: public For_AST{
public:
While_AST(Expr_AST* Expr, Block_AST* Block)
    : For_AST(new ExprList_AST(new ExprList_AST(0, Expr), 0), Block)
    {
	if (option_Debug)
	    std::cout << "\tcreated While_AST\n";
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

#endif
