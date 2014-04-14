/********************************************************************
* ast.h - AST for Decaf
*
* Note: I am not entirely sure why accept() members which should be
*       default inherited from Node_AST's need to be explicitly 
*       implemented in classes that do not change the Node_AST default
*
********************************************************************/

#ifndef AST_H_
#define AST_H_

#include <string>
#include <map>
#include <sstream>
#include "lexer.h"

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
class LogicalExpr_AST;
class OrExpr_AST;
class AndExpr_AST;
class RelExpr_AST;
class NotExpr_AST;

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
    virtual void visit(LogicalExpr_AST*) = 0;
    virtual void visit(OrExpr_AST*) = 0;
    virtual void visit(AndExpr_AST*) = 0;
    virtual void visit(RelExpr_AST*) = 0;
    virtual void visit(NotExpr_AST*) = 0;

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
    : parent_(0), lChild_(lC), rChild_(rC), line_(lineNo), col_(colNo),
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
* Expression parent class
***************************************/

// arithmetic, logical, basic, and access (array) types
class Expr_AST: public Node_AST{ // ***TO DO: link properly when ready
public:
Expr_AST(token Type=token(), token OpTor=token(), 
	 Node_AST* lc=0, Node_AST* rc=0)
    : Node_AST(lc, rc), type_(Type), op_(OpTor)
    {
	if ( ( "" != type_.Lex() ) ){ // in case of default constructor
	    typeW_ = setWidth();
	    typeP_ = setPriority();
	}
	else
	    typeW_ = typeP_ = -1;
	std::cout << "\tcreated Expr with op = " << op_.Lex();
	std::cout << ", type = " << type_.Lex() << "\n";
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

    // for use in its array grand-child
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
	std::cout << "\tcreated tmp = " << addr_ << "\n";
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
	std::cout << "\tcreated an Id = " << addr_ << "\n";
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
	std::cout << "\tcreated FltExpr with value = " << addr_ << "\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// ******TO DO: STRING (etc.)********************

/***************************************
* Expression Non-logical children
***************************************/

// Upon creating this object, LHS->TypeP() == RHS->TypeP()
// Pre recommendation of Effective C++, don't throw error in ctor, though.
class ArithmExpr_AST: public Expr_AST{
public:
ArithmExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), Op, LHS, RHS) 
    {
	std::cout << "\tcreated ArithmExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
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

// replaces old position of LHS in AST, with new LC being the TMP
class CoercedExpr_AST: public Expr_AST{
public:
CoercedExpr_AST(Expr_AST* TMP, Expr_AST* Expr)
    : Expr_AST(token(TMP->Type()), token(tok_eof), TMP, Expr),
	from_(Expr->Type().Tok()), to_(TMP->Type().Tok())
    {
	std::cout << "\tcreated CoercedExpr with type = " 
		  << type_.Lex() << "\n";
    }

    tokenType From() const { return from_; }
    tokenType To() const {return to_; }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }

private:
    tokenType from_; // for easier access by visitor
    tokenType to_;
};

class UnaryArithmExpr_AST: public Expr_AST{
public:
UnaryArithmExpr_AST(token Op, Expr_AST* RHS)
    : Expr_AST(token(RHS->Type()), Op, 0, RHS)
    {
	std::cout << "\tcreated Unary ArithmExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
    }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }

};


/***************************************
* Expression Logical children
***************************************/

class LogicalExpr_AST: public Expr_AST{
public:
LogicalExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), Op, LHS, RHS) 
    {
	std::cout << "\tcreated LogicalExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
    }

    ~LogicalExpr_AST() {}

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
	Visitor->visit(this);
    }

};

// implement ||
class OrExpr_AST: public LogicalExpr_AST{
public:
OrExpr_AST(Expr_AST* LHS, Expr_AST* RHS)
    : LogicalExpr_AST(token(tok_log_or), LHS, RHS) 
    {
	std::cout << "\tcreated OrExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
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

// implement && (as, in ultimate code generation, we short-circuit, it 
// makes sense to track this differently from ||)
class AndExpr_AST: public LogicalExpr_AST{
public:
AndExpr_AST(Expr_AST* LHS, Expr_AST* RHS)
    : LogicalExpr_AST(token(tok_log_and), LHS, RHS) 
    {
	std::cout << "\tcreated AndExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
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

// implement <, <=, >, >=, ==, !=
class RelExpr_AST: public LogicalExpr_AST{
public:
RelExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : LogicalExpr_AST(Op, LHS, RHS) 
    {
	std::cout << "\tcreated RelExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
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

// implement (logical) ! (chaining not allowed)
class NotExpr_AST: public LogicalExpr_AST{
public:
NotExpr_AST(token(tok_log_not), Expr_AST* LHS)
    : LogicalExpr_AST(token(tok_log_not), LHS, 0)
    {
	std::cout << "\tcreated NotExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
    }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	Visitor->visit(this);
    }

};

#endif
