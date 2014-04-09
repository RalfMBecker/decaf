/********************************************************************
* ast.h - AST for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*
* Error checking: we assume parser hands on only arguments already
*                 checked for correctness
*
********************************************************************/

#ifndef AST_H_
#define AST_H_

#include <string>
#include <sstream>
#include "lexer.h"

int typePriority(std::string);
int typeWidth(std::string);

/***************************************
* Base classes
***************************************/
// parents manage their children: once they are created, add a pointer
// to parent in any child
class NodeAST{
public:
NodeAST(NodeAST* lC=0, NodeAST* rC=0)
    : parent_(0), lChild_(lC), rChild_(rC), line_(lineNo), col_(colNo)
    {
	if ( (0 != lChild_) )
	    lChild_->setParent(this);
	if ( (0 != rChild_) )
	    rChild_->setParent(this);
    }

    ~NodeAST() {}

    virtual void printLabel(void) { std::cout << "L" << label_Count_++ << ":"; }

    virtual int Line(void) const { return line_; }
    virtual int Col(void) const { return col_; }

    virtual NodeAST* Parent(void) const { return parent_; }
    virtual NodeAST* LChild(void) const { return lChild_; }
    virtual NodeAST* RChild(void) const { return rChild_; }
    void setParent(NodeAST* Par) { parent_ = Par; }

private:
    NodeAST* parent_;
    NodeAST* lChild_;
    NodeAST* rChild_;
    int line_;
    int col_;
    static int label_Count_;
};

// arithmetic, logical, and basic types
class ExprAST: public NodeAST{
public:
ExprAST(token Type, token OpTor, NodeAST* lc=0, NodeAST* rc=0)
    : NodeAST(lc, rc), type_(Type.Lex()), op_(OpTor)
    {
	typeW_ = typeWidth(type_);
	typeP_ = typePriority(type_);
    }
    ~ExprAST() {}

    friend int typePriority(std::string);
    friend int typeWidth(std::string);

    std::string Type(void) const { return type_;}
    token Op(void) const { return op_; }
    int TypeW(void) const { return typeW_; }
    int TypeP(void) const { return typeP_; }

private:
    std::string type_;
    token op_;
    int typeW_;
    int typeP_;
};

// Serves to handle unified code generation (children: logical/arithm.):
// Handles creation and assignment to a temp variable (SSA style)
class OpAST: public ExprAST{
public:
OpAST(token Type, token Op, NodeAST* lc=0, NodeAST* rc=0)
    : ExprAST(Type, Op, lc, rc) {}
// methods differ from those for Expr, hence separate. TO DO

};

/***************************************
* Expression terminals
***************************************/

class TempAST: public ExprAST{
public:
TempAST(token Type, token Op)
    : ExprAST(Type, Op, 0, 0)
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

class IdExprAST: public ExprAST{
public:
IdExprAST(token Type, token Op)
    : ExprAST(Type, Op, 0, 0) { name_ = Op.Lex(); }

    std::string Name(void) const { return name_; }

private: 
    std::string name_; // for easier access
};

class IntExprAST: public ExprAST{
public:
IntExprAST(token Op)
    : ExprAST(token(tok_int), Op, 0, 0) { value_ = Op.Lex(); }

    std::string Value(void) const { return value_;}

private:
    std::string value_; // for easier access
};

class FltExprAST: public ExprAST{
public:
FltExprAST(token Op)
    : ExprAST(token(tok_double), Op, 0, 0) { value_ = Op.Lex(); }

    std::string Value(void) const { return value_; }

private:
    std::string value_; // for easier access
};

// ******TO DO: STRING (etc.)********************

/***************************************
* Arithmetic expressions
***************************************/

class ArithmExprAST: public OpAST{
public:
    ArithmExprAST(token Op, ExprAST* LHS, ExprAST* RHS)
	: OpAST(token(), Op, LHS, RHS) {}

};

#endif
