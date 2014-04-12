/********************************************************************
* ast.h - AST for Decaf
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

/***************************************
* Base class
***************************************/
// parents manage their children: once they are created, add a pointer
// to parent in any child
class Node_AST{
public:
Node_AST(Node_AST* lC=0, Node_AST* rC=0)
    : parent_(0), lChild_(lC), rChild_(rC), line_(lineNo), col_(colNo)
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

    void setParent(Node_AST* Par) { parent_ = Par; }

protected:
    Node_AST* parent_;
    Node_AST* lChild_;
    Node_AST* rChild_;
    int line_;
    int col_;
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
	name_ = tmp.str();
	op_.SetTokenLex(name_);
	std::cout << "\tcreated tmp = " << name_ << "\n";
    }

    std::string Name(void) const { return name_; }

private:
    std::string name_; // for easier access
    static int count_;
};

// ***parts of a fct declaration can probably be folded into this
class IdExpr_AST: public Expr_AST{
public:
IdExpr_AST(token Type, token Op)
    : Expr_AST(Type, Op, 0, 0) 
    { 
	name_ = Op.Lex();
	std::cout << "\tcreated an Id = " << name_ << "\n";
    }

    virtual std::string Name(void) const { return name_; }

protected: 
    std::string name_; // for easier access
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
	std::cout << "\tcreated an array with Id = " << name_  << "\n";
    }

    int Size(void) const { return size_; }

private:
    int size_;
};

class IntExpr_AST: public Expr_AST{
public:
IntExpr_AST(token Op)
    : Expr_AST(token(tok_int), Op, 0, 0) 
    {
	value_ = Op.Lex();
	std::cout << "\tcreated IntExpr with value = " << value_ << "\n";
    }

    std::string Value(void) const { return value_;}

private:
    std::string value_; // for easier access
};

class FltExpr_AST: public Expr_AST{
public:
FltExpr_AST(token Op)
    : Expr_AST(token(tok_double), Op, 0, 0) 
    {
	value_ = Op.Lex(); 
	std::cout << "\tcreated FltExpr with value = " << value_ << "\n";
    }

    std::string Value(void) const { return value_; }

private:
    std::string value_; // for easier access
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
};

// replaces old position of LHS, with new LC being the TMP
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

private:
    tokenType from_; // for easier access by visitor
    tokenType to_;
};

class UnaryArithmExpr_AST: public Expr_AST{
public:
UnaryArithmExpr_AST(token Op, Expr_AST* LHS)
    : Expr_AST(token(LHS->Type()), Op, LHS)
    {
	std::cout << "\tcreated Unary ArithmExpr with op = " << op_.Lex() 
		  << ", type = " << type_.Lex() << "\n";
    }
};



/***************************************
* Expression Logical children
***************************************/

#endif
