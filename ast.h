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
extern std::map<std::string, int> type_PrecTable;
extern std::map<std::string, int> type_WidthTable;

/***************************************
* Base classes
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

    virtual void printLabel(void) { std::cout << "L" << label_Count_++ << ":"; }

    virtual int Line(void) const { return line_; }
    virtual int Col(void) const { return col_; }

    virtual Node_AST* Parent(void) const { return parent_; }
    virtual Node_AST* LChild(void) const { return lChild_; }
    virtual Node_AST* RChild(void) const { return rChild_; }
    void setParent(Node_AST* Par) { parent_ = Par; }

private:
    Node_AST* parent_;
    Node_AST* lChild_;
    Node_AST* rChild_;
    int line_;
    int col_;
    static int label_Count_;
};

// arithmetic, logical, basic, and access (array) types
class Expr_AST: public Node_AST{
public:
Expr_AST(token Type=token(), token OpTor=token(), 
	 Node_AST* lc=0, Node_AST* rc=0)
    : Node_AST(lc, rc), type_(Type.Lex()), op_(OpTor)
    {
	if ( ( "" != type_ ) ){ // in case of default constructor
	    typeW_ = setWidth();
	    typeP_ = setPriority();
	}
	else
	    typeW_ = typeP_ = -1;
    }
    ~Expr_AST() {}

    virtual int setWidth(void)
    {
	if ( (type_WidthTable.end() != type_WidthTable.find(type_)) )
	    return type_WidthTable[type_];
	else
	    return -1;
    }

    int setPriority(void)
    {
	if ( (type_PrecTable.end() != type_PrecTable.find(type_)) )
	    return type_PrecTable[type_];
	else
	    return -1;
    }

    // for use in its array grand-child
    virtual void forceWidth(int w) { typeW_ = w; }
	
    std::string Type(void) const { return type_;}
    token Op(void) const { return op_; }
    int TypeW(void) const { return typeW_; }
    int TypeP(void) const { return typeP_; }

protected:
    std::string type_;
    token op_;
    int typeW_;
    int typeP_;
};

/***************************************
* Expression terminals
***************************************/

class Temp_AST: public Expr_AST{
public:
Temp_AST(token Type, token Op)
    : Expr_AST(Type, Op, 0, 0)
    {
	std::stringstream tmp;
	tmp << "t" << ++count_;
	name_ = tmp.str();
	Op.SetTokenLex(name_);
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
    : Expr_AST(Type, Op, 0, 0) { name_ = Op.Lex(); }

    virtual std::string Name(void) const { return name_; }

private: 
    std::string name_; // for easier access
};

// ***TO DO: revisit when used (in progress)***
class IdArrayExpr_AST: public IdExpr_AST{
public:
    IdArrayExpr_AST(token Type, token Op, int Size)
	: IdExpr_AST(Type, Op), size_(Size)
    {
	if ( (-1 != typeW_) )
	    typeW_ *= size_;
    }

    int Size(void) const { return size_; }

private:
    int size_;
};

class IntExpr_AST: public Expr_AST{
public:
IntExpr_AST(token Op)
    : Expr_AST(token(tok_int), Op, 0, 0) { value_ = Op.Lex(); }

    std::string Value(void) const { return value_;}

private:
    std::string value_; // for easier access
};

class FltExpr_AST: public Expr_AST{
public:
FltExpr_AST(token Op)
    : Expr_AST(token(tok_double), Op, 0, 0) { value_ = Op.Lex(); }

    std::string Value(void) const { return value_; }

private:
    std::string value_; // for easier access
};

class BoolExpr_AST: public Expr_AST{
public:
BoolExpr_AST(token Op) // Op = "false" "true" (tok_true tok_false)
    : Expr_AST(token(tok_bool), Op, 0, 0) { value_ = Op.Lex(); }

    std::string Value(void) const { return value_; }

private:
    std::string value_; // for easier access
};

// ******TO DO: STRING (etc.)********************

/***************************************
* Expression Arithmetic children
***************************************/

class ArithmExpr_AST: public Expr_AST{
public:
    ArithmExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
	: Expr_AST(token(), Op, LHS, RHS) {}

};

/***************************************
* Expression Logical children
***************************************/

/*
class UnaryArithmExpr_AST: public ArithmExpr_AST{
public:
    UnaryArithmExpr_AST(token Op, Expr_AST* LHS)
    {

    }


private:

};
*/

/***************************************
* TmpForExpr and its children - TO DO: likely will be removed
***************************************/

// Serves to handle unified code generation:
// Handles creation and assignment to a temp variable (SSA style)
class TmpForExpr_AST: public Expr_AST{
public:
TmpForExpr_AST(token Type, token Op, Node_AST* lc=0, Node_AST* rc=0)
    : Expr_AST(Type, Op, lc, rc) {}
// methods differ from those for Expr, hence separate. TO DO

    ~TmpForExpr_AST() {};

};

#endif
