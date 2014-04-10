/********************************************************************
* ast.h - AST for Decaf
*
* BinOp Parsing: using Operator Precedence parsing (bin_OpTable)
*                (also known as Dijksta-Shunting algorithm)
*
* Error checking: we assume parser hands on only arguments already
*                 checked for correctness
*
********************************************************************/

#ifndef AST_H_
#define AST_H_

// ****** TO DO: DEPENDENCY ANALYSIS - UNTANGLE CLASSES ******
class ExprAST;
class IdExprAST; // forward declare what is used in tables.h

#include <string>
#include <sstream>
#include "lexer.h"
#include "tables.h"

// ****** TO DO: DEPENDENCY ANALYSIS - UNTANGLE CLASSES ******
extern std::map<std::string, int> type_PrecTable;
extern std::map<std::string, int> type_WidthTable;
int typePriority(std::string const&);
int typeWidth(std::string const&);


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

// arithmetic, logical, basic, and access (array) types
class ExprAST: public NodeAST{
public:
ExprAST(token Type = token(), token OpTor=token(), NodeAST* lc=0, NodeAST* rc=0)
    : NodeAST(lc, rc), type_(Type.Lex()), op_(OpTor)
    {
	if ( ( "" != type_ ) ){ // in case of default constructor
	    typeW_ = typeWidth(Type.Lex());
	    typeP_ = typePriority(Type.Lex());
	}
	else
	    typeW_ = typeP_ = 0;
    }
    ~ExprAST() {}

//    friend int typePriority(std::string const&);
//    friend int typeWidth(std::string const&);

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
* TmpForExpr and its children
***************************************/

// Serves to handle unified code generation:
// Handles creation and assignment to a temp variable (SSA style)
class TmpForExprAST: public ExprAST{
public:
TmpForExprAST(token Type, token Op, NodeAST* lc=0, NodeAST* rc=0)
    : ExprAST(Type, Op, lc, rc) {}
// methods differ from those for Expr, hence separate. TO DO

};

class ArithmExprAST: public TmpForExprAST{
public:
    ArithmExprAST(token Op, ExprAST* LHS, ExprAST* RHS)
	: TmpForExprAST(token(), Op, LHS, RHS) {}

};

/*
class UnaryArithmExprAST: public ArithmExprAST{
public:
    UnaryArithmExprAST(token Op, ExprAST* LHS)
    {

    }


private:

};
*/

#endif
