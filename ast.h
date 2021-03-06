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
#include <vector>
#include <sstream>
#include <cstdlib>
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
class PreIncrIdExpr_AST;
class PostIncrIdExpr_AST;
class ArrayIdExpr_AST;
class PreIncrArrayIdExpr_AST;
class PostIncrArrayIdExpr_AST;
class IntExpr_AST;
class FltExpr_AST;
class ArithmExpr_AST;
class CoercedExpr_AST;
class UnaryArithmExpr_AST;
class AssignExpr_AST;
class ModAssignExpr_AST;
class LogicalExpr_AST;
class OrExpr_AST;
class AndExpr_AST;
class RelExpr_AST;
class NotExpr_AST;
class NOP_AST;

class Block_AST;
class StmtList_AST;
class Stmt_AST;
class EOB_AST;
class VarDecl_AST;
class ArrayVarDecl_AST;
class Assign_AST;
class ModAssign_AST;
class IfType_AST;
class If_AST;
class For_AST;
class Break_AST;
class Cont_AST;

class AST_Visitor{
public: 
    virtual void visit(Tmp_AST*) = 0;
    virtual void visit(IdExpr_AST*) = 0;
    virtual void visit(PreIncrIdExpr_AST*) = 0;
    virtual void visit(PostIncrIdExpr_AST*) = 0;
    virtual void visit(ArrayIdExpr_AST*) = 0;
    virtual void visit(PreIncrArrayIdExpr_AST*) = 0;
    virtual void visit(PostIncrArrayIdExpr_AST*) = 0;
    virtual void visit(IntExpr_AST*) = 0;
    virtual void visit(FltExpr_AST*) = 0;
    virtual void visit(ArithmExpr_AST*) = 0;
    virtual void visit(CoercedExpr_AST*) = 0;
    virtual void visit(UnaryArithmExpr_AST*) = 0;
    virtual void visit(AssignExpr_AST* V) = 0;
    virtual void visit(OrExpr_AST*) = 0;
    virtual void visit(AndExpr_AST*) = 0;
    virtual void visit(RelExpr_AST*) = 0;
    virtual void visit(NotExpr_AST*) = 0;
    virtual void visit(NOP_AST*) = 0;

    virtual void visit(EOB_AST*) = 0;
    virtual void visit(VarDecl_AST*) = 0;
    virtual void visit(ArrayVarDecl_AST*) = 0;
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
// Parents manage their children: once they are created, add a pointer
// to parent in any child.
// As we have partial DAG features (eg., all expressions with a variable
// child share this same object), we use hand-written refcounting to
// manage de-allocation).
class Node_AST{
public:
Node_AST(Node_AST* lC = 0, Node_AST* rC = 0)
    : parent_(0), lChild_(lC), rChild_(rC), line_(line_No), col_(col_No),
	addr_(""), env_(top_Env)
    {
	if ( (0 != lChild_) ){
	    lChild_->setParent(this);
	    lChild_->RefCountPlus();
	}
	if ( (0 != rChild_) ){
	    rChild_->setParent(this);
	    rChild_->RefCountPlus();
	}
	ref_Count_ = 1;
    }

    ~Node_AST() {}

    virtual std::string Addr(void) { return addr_; } // not const
    // as re-defined in IdExpr_AST, where it is not const

    int Line(void) const { return line_; }
    int Col(void) const { return col_; }
    Node_AST* Parent(void) const { return parent_; }
    Node_AST* LChild(void) const { return lChild_; }
    Node_AST* RChild(void) const { return rChild_; }
    Env* getEnv(void) const { return env_; }

    void setParent(Node_AST* Par) { parent_ = Par; }
    void setAddr(std::string Addr) { addr_ = Addr; }

    int RefCount(void) const { return ref_Count_; }
    void RefCountPlus(void) { ref_Count_++; }
    void RefCountMinus(void) { ref_Count_--; }

    virtual void accept(AST_Visitor* Visitor)
    {
	if ( (0!= this->lChild_) )
	    this->lChild_->accept(Visitor);
	if ( (0!= this->rChild_) )
	    this->rChild_->accept(Visitor);
    }

protected: // we don't really use protected variables in children
    Node_AST* parent_;
    Node_AST* lChild_;
    Node_AST* rChild_;
    int line_;
    int col_;
    std::string addr_;
    Env* env_;
    static int label_Count_;
    int ref_Count_; // Objects might be a child of several other objects
                    // (this matters when de-allocating memory)
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
//	Visitor->visit(this); // as we visit all children and children of
	// children first, then would visit the StmtList proper AFTER, and
	// we do nothing, it only confuses in -d p/o (si does nothing below).
        //      sl                 
	//   lc1  s2                 -> visit lc1-lc2-rc1-s2-s1
	//      lc2 rc1
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
    }
};


// ** TO DO: REFACTOR - this could be key to manage better tmp allocation
// ******************************************************
// End of Block marker (used to clean up after arrays with integer
// expression bounds)
class EOB_AST: public Stmt_AST{
public:
EOB_AST(void)
    : Stmt_AST(0, 0)
    {
	if (option_Debug) std::cout << "\tcreated an EOB_AST...\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
}; 

/****************************************************
* Expression parent class, and lists of expressions
*****************************************************/
// Arithmetic, logical, basic, and access (array) types
class Expr_AST: public Stmt_AST{
public:
Expr_AST(token Type=token(), token OpTok = token(), Expr_AST* lc=0, 
	 Expr_AST* rc=0)
    : Stmt_AST(lc, rc), type_(Type), op_(OpTok)
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

    int setWidth(void)
    {
	if ( (typeWidth_Table.end() != typeWidth_Table.find(type_.Lex())) )
	    return typeWidth_Table[type_.Lex()];
	else
	    return -1;
    }

    int setPriority(void)
    {
	if ( (typePrec_Table.end() != typePrec_Table.find(type_.Lex())) )
	    return typePrec_Table[type_.Lex()];
	else
	    return -1;
    }

    // for use in classes and arrays (re-defined in a descendant)
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
    }

protected:
    token type_; // (tok_int, "int")
    token op_;   // (tok_plus, "+")
    int typeW_;
    int typeP_;
};

// 3 (or less) expressions used in cond part of iteration types
class IterExprList_AST: public Node_AST{
public:
IterExprList_AST(Expr_AST* E1 = 0, Expr_AST* E2 = 0, Expr_AST* E3 = 0) 
    : Node_AST(), init_(E1), cond_(E2), iter_(E3)
    {
	if (option_Debug) std::cout << "\tcreated an iterExprList...\n";
    }

    ~IterExprList_AST()
    {
	if ( (0 != init_) ) delete init_;
	if ( (0 != cond_) ) delete cond_;
	if ( (0 != iter_) ) delete iter_;
    }

    Expr_AST* Init(void) const { return init_; }
    Expr_AST* Cond(void) const { return cond_; }
    Expr_AST* Iter(void) const { return iter_; }

private:
    Expr_AST* init_;
    Expr_AST* cond_;
    Expr_AST* iter_;
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

class IdExpr_AST: public Expr_AST{
public:
IdExpr_AST(token Type, token Op, int I = 0, int W = 0)
    : Expr_AST(Type, Op, 0, 0), initialized_(I), warning_Emitted_(W), 
	tmp_Addr_("")
    { 
	setAddr(Op.Lex());
	if (option_Debug) std::cout << "\tcreated an Id = " << addr_ << "\n";
    }

    int isInitialized(void) const { return initialized_; }
    void Initialize(void) { initialized_ = 1; }

    int WarningEmitted(void) const { return warning_Emitted_; }
    void Warned(void) { warning_Emitted_ = 1; }

    std::string TmpAddr(void) const { return tmp_Addr_; }
    void setTmpAddr(std::string A) { tmp_Addr_ = A; } 

    std::string Addr(void)
    {
	std::string ret = Node_AST::Addr();

	if ( ("" != TmpAddr()) ){
	    ret = TmpAddr();
	    setTmpAddr("");
	}

	return ret;
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    int initialized_;
    int warning_Emitted_;
    std::string tmp_Addr_; // used for Incr type descendants
};

// C99 6.5 (2): Between sequence points, value cannot be read and stored.
//              Taken literally, this disallows ++a; a++;, but a FAQ 
//              clarifies intent (it is legal).
//              Sequence points (SP's) are declared to delineate what we call
//              an AssignExpr_AST (among others); C does not have our
//              Assign_AST (probably simplifies definition of SP's, say).
// ** TO DO: monitor - if we add more types, only ints and double can be 
//           pre- and post-incremented
class PreIncrIdExpr_AST: public IdExpr_AST{
public:
    PreIncrIdExpr_AST(IdExpr_AST* P, int V)
	: IdExpr_AST(P->Type(), P->Op()), name_(P), inc_Value_(V)
    {
	if (option_Debug){
	    std::ostringstream tmp_Stream;
	    if ( (0 < V) )
		tmp_Stream << "++";
	    else
		tmp_Stream << "--";
	    tmp_Stream << (P->Op()).Lex();

	    std::cout << "\tcreated PreIncrIdExpr_AST (";
	    std::cout << tmp_Stream.str() << ")\n";;
	}
    }

    IdExpr_AST* Name(void) const { return name_; } 
    int IncValue(void) const { return inc_Value_; }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    IdExpr_AST* name_; // we need to stay linked to the object tracking
    int inc_Value_;    // initialization etc. 
};

class PostIncrIdExpr_AST: public IdExpr_AST{
public:
    PostIncrIdExpr_AST(IdExpr_AST* P, int V)
	: IdExpr_AST(P->Type(), P->Op()), name_(P), inc_Value_(V)
    {
	if (option_Debug){
	    std::ostringstream tmp_Stream;
	    tmp_Stream << (P->Op()).Lex();
	    if ( (0 < V) )
		tmp_Stream << "++";
	    else
		tmp_Stream << "--";

	    std::cout << "\tcreated PostIncrIdExpr_AST (";
	    std::cout << tmp_Stream.str() << ")\n";;
	}
    }

    IdExpr_AST* Name(void) const { return name_; } 
    int IncValue(void) const { return inc_Value_; }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    IdExpr_AST* name_;
    int inc_Value_;
};

class ArrayVarDecl_AST;

// As we don't check for initialization before access in the array case,
// set up status variables to reflect this.
// Handing on N (which could be read as B->Expr()) separately to avoid 
// changing order of definitions (would need full definition of
// ArrayVarDecl_AST, not only forward declaration).
// C99 6.3.2.1 (3): handled similarly - treated as if base is a reference
//                  to base, with element access calculated as an offset
//                  from that base
class ArrayIdExpr_AST: public IdExpr_AST{
public:
ArrayIdExpr_AST(ArrayVarDecl_AST* B, IdExpr_AST* N, int AI, 
		std::vector<Expr_AST*>* Access, 
		std::vector<std::string>* Final = 0, std::string A = "")
    : IdExpr_AST(N->Type(), N->Op()), base_(B), base_Id_(N), 
	all_IntVals_(AI), dims_(Access), dims_Final_(Final)
    {
	if (AI){
	    std::ostringstream tmp_Stream;
	    tmp_Stream << "(-$" <<  A << ")" << N->Addr();
	    setAddr(tmp_Stream.str());
	}
	num_Dims_ = dims_->size();

	if (option_Debug){
	    std::cout << "\tcreated ArrayIdExpr_AST (";
	    if (all_IntVals_)
		std::cout << Addr() << ")\n";
	    else
		std::cout << "access compile-time resolved)\n";
	}
    }

ArrayIdExpr_AST(const ArrayIdExpr_AST& r)
    : IdExpr_AST( *(r.BaseId()) ) // use default copy ctor
    {
	base_ = r.Base();
	base_Id_ = r.BaseId();
	all_IntVals_ = r.allInts();
	dims_ = r.Dims();
	dims_Final_ = r.DimsFinal();
    }

    ~ArrayIdExpr_AST()
    {
	if ( (0 != dims_) ) delete dims_; 
	if ( (0 != dims_Final_) ) delete dims_Final_; 
//	if ( (0 != base_) ) delete base_; // to use delete operator on base_,
	// a forward declaration of ArrayVarDecl_AST is not enough. We would
	// need to physically put the entire class ahead, which doesn't
	// fit the logical structure of this file. Hence, we accept a 
	// memory leak for base_ (which will be cleaned up after 'exit')
    }

    ArrayVarDecl_AST* Base(void) const { return base_; } 
    IdExpr_AST* BaseId(void) const { return base_Id_; }
    int allInts(void) const { return all_IntVals_; }
    std::vector<Expr_AST*>* Dims(void) const { return dims_;}
    int numDims(void) const { return num_Dims_; }

    std::vector<std::string>* DimsFinal(void) const { return dims_Final_;}
    void addToDimsFinalEnd(std::string V) { dims_Final_->push_back(V); }
    void addToDimsFinalFront(std::string V) 
    {
	dims_Final_->insert(dims_Final_->begin(), V);
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    ArrayVarDecl_AST* base_; // as declared
    IdExpr_AST* base_Id_; 
    int all_IntVals_; // if 1, have names (strings) in dims_Final_
    std::vector<Expr_AST*>* dims_; // access encoded in expressions
    std::vector<std::string>* dims_Final_; // filled in by visitor sometimes
    int num_Dims_;
};

class PreIncrArrayIdExpr_AST: public ArrayIdExpr_AST{
public:
PreIncrArrayIdExpr_AST(ArrayIdExpr_AST* P, int V)
    : ArrayIdExpr_AST(*P), name_(P), inc_Value_(V)
    { 
	if (option_Debug){
	    std::ostringstream tmp_Stream;
	    if ( (0 < V) )
		tmp_Stream << "++";
	    else
		tmp_Stream << "--";
	    tmp_Stream << (P->Op()).Lex();
	    for (int i = 0; i < P->numDims(); i++)
		tmp_Stream << "[]";

	    std::cout << "\tcreated PreIncrArrayIdExpr_AST (";
	    std::cout << tmp_Stream.str() << ")\n";;
	}
    }

    ArrayIdExpr_AST* Name(void) const { return name_; } 
    int IncValue(void) const { return inc_Value_; }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    ArrayIdExpr_AST* name_; // we need to stay linked to variable object
    int inc_Value_; 
};

class PostIncrArrayIdExpr_AST: public ArrayIdExpr_AST{
public:
PostIncrArrayIdExpr_AST(ArrayIdExpr_AST* P, int V)
    : ArrayIdExpr_AST(*P), name_(P), inc_Value_(V)
    { 
	if (option_Debug){
	    std::ostringstream tmp_Stream;
	    tmp_Stream << (P->Op()).Lex();
	    for (int i = 0; i < P->numDims(); i++)
		tmp_Stream << "[]";
	    if ( (0 < V) )
		tmp_Stream << "++";
	    else
		tmp_Stream << "--";

	    std::cout << "\tcreated PostIncrArrayIdExpr_AST (";
	    std::cout << tmp_Stream.str() << ")\n";;
	}
    }

    ArrayIdExpr_AST* Name(void) const { return name_; } 
    int IncValue(void) const { return inc_Value_; }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    ArrayIdExpr_AST* name_; // we need to stay linked variable object
    int inc_Value_;
};

// Type: tok_int; Op.Tok() = tok_intV (similar for flt, string)
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

// C99 6.4.5 (4) - string literals:
//               - append a '\0' at end (done in pre-processing)
//               - assign to static storage (aka, .data segment)
// We allocate in .data section, so the object is addressless at this level.
class String_AST: public Expr_AST{
public:
String_AST(token Op)
    : Expr_AST(token(tok_string), Op, 0, 0) 
    {
	if (option_Debug)
	    std::cout << "\tcreated StringExpr \"" << Op.Lex()  << "\"\n";
    }

//    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
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

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// replaces old position of LHS in AST, with new LC being the TMP
// C99 6.3.1.4 (1) truncate float -> int case
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

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

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

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// ** TO DO: initialization is too simplistic
class AssignExpr_AST: public Expr_AST{
public:
AssignExpr_AST(IdExpr_AST* Id, Expr_AST* Expr)
    : Expr_AST(token(Id->Type()), token(tok_eq), Id, Expr) 
    {
	setAddr(Id->Op().Lex());
	Id->Initialize();
	if (option_Debug)
	    std::cout << "\tcreated AssignExpr_AST with LHS = "<< addr_<< "\n";
    }

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

class ModAssignExpr_AST: public AssignExpr_AST{
public:
ModAssignExpr_AST(IdExpr_AST* Id, Expr_AST* Expr, token Type)
    : AssignExpr_AST(Id, Expr), type_(Type)
    {
	setAddr(Id->Op().Lex());
	Id->Initialize();
	if (option_Debug)
	    std::cout << "\tcreated AssignExpr_AST with LHS = "<< addr_<< "\n";
    }

    token ModType(void) const { return type_; }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    token type_;
};

/***************************************
* Expression Logical children
***************************************/
// implement ||
class OrExpr_AST: public Expr_AST{
public:
OrExpr_AST(Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), token(tok_log_or), LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated OrExpr with op = " << op_.Lex();
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// implement && (as, in ultimate code generation, we short-circuit, it 
// makes sense to track this different from ||)
class AndExpr_AST: public Expr_AST{
public:
AndExpr_AST(Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), token(tok_log_and), LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated AndExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// implement <, <=, >, >=, ==, !=
class RelExpr_AST: public Expr_AST{
public:
RelExpr_AST(token Op, Expr_AST* LHS, Expr_AST* RHS)
    : Expr_AST(token(LHS->Type()), Op, LHS, RHS) 
    {
	if (option_Debug){
	    std::cout << "\tcreated RelExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

};

// implement (logical) !
class NotExpr_AST: public Expr_AST{
public:
NotExpr_AST(Expr_AST* LHS)
    : Expr_AST(token(LHS->Type()), token(tok_log_not), LHS, 0)
    {
	if (option_Debug){
	    std::cout << "\tcreated NotExpr with op = " << op_.Lex(); 
	    std::cout << ", type = " << type_.Lex() << "\n";
	}
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
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

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class Cont_AST: public Stmt_AST{
public:
Cont_AST(void)
    : Stmt_AST(0, 0)
    {
	if (option_Debug) std::cout<< "\tcreated a Cont_AST\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// parent class for declarations:
// - basic type variables & arrays
// - functions
// - classes
class Decl_AST: public Stmt_AST{
public:
Decl_AST(IdExpr_AST* Id)
    : Stmt_AST(Id, 0), name_( (Id->Op()).Lex() ), type_(Id->Type()), 
	width_(Id->TypeW()), expr_(Id)
    {
	setAddr(Id->Op().Lex());
	if (option_Debug)
	    std::cout<< "\tcreated Decl_AST with name = " << addr_ << "\n";
    }

    ~Decl_AST() {}

    std::string Name(void) const { return name_; }
    token Type(void) const { return type_; }

    int Width(void) const { return width_; }
    IdExpr_AST* Expr(void) const { return expr_; }
    void forceWidth(int W) { width_ = W; } // ** TO DO: seems redundant

private:
    std::string name_; // all vars for easier access only
    token type_;
    int width_;
    IdExpr_AST* expr_; // to avoid some casts
};


class VarDecl_AST: public Decl_AST{
public:
VarDecl_AST(IdExpr_AST* Id)
    : Decl_AST(Id)
    {
	if (option_Debug) std::cout<< "\tcreated VarDecl_AST....\n";
    }

    ~VarDecl_AST() {}

    virtual void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

// Access: bound-checked 
// Bounds: integer expressions allowed -
//         if all integers, can add at compile-time
//         if not, adjust stack at run-time (handled in visitor)
// Note:   for one file only, there is no need for a true run-time check:
//         at least theoretically, we could calculate if the dimension
//         expressions evaluate to a positive integer. The implementation
//         chosen extends to run-time error checks after a linker is
//         (hypothetically) added (e.g., extern variables from other files 
//         filled in). 
class ArrayVarDecl_AST: public VarDecl_AST{
public:
ArrayVarDecl_AST(IdExpr_AST* Name, std::vector<Expr_AST*>* D, int I, int W )
    : VarDecl_AST(Name), dims_(D), all_IntVals_(I)
    {
	num_Dims_ = dims_->size();

	dims_Final_ = new std::vector<std::string>;
	dims_Final_->reserve(num_Dims_);
	if (all_IntVals_){
	    std::vector<Expr_AST*>::const_iterator iter;
	    for ( iter = D->begin(); iter != D->end(); iter++ )
		dims_Final_->push_back( (*iter)->Addr() );
	}

	this->forceWidth(W);

	if (option_Debug){
	    std::cout << "\tcreated ArrayVarDecl_AST with name = " << addr_;
	    std::cout << ", " << num_Dims_ << " dimensions, ";
	    if (all_IntVals_){
		std::cout << "compile-time allocated with width ";
		std::cout << this->Width() << "\n";
	    }
	    else
		std::cout << "and expression bounds (allocated at run-time)\n";
	}
    }

    ~ArrayVarDecl_AST() 
    {
	if ( (0 != dims_) ) delete dims_;
	if ( (0 != dims_Final_) ) delete dims_Final_;
    }

    int allInts(void) const { return all_IntVals_; }
    int numDims(void) const { return num_Dims_; }
    std::vector<Expr_AST*>* Dims(void) const { return dims_; }

    std::vector<std::string>* DimsFinal(void) const {return dims_Final_;}
    virtual void addToDimsFinalEnd(std::string V) { dims_Final_->push_back(V); }
    virtual void addToDimsFinalFront(std::string V) 
    {
	dims_Final_->insert(dims_Final_->begin(), V);
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

private:
    std::vector<Expr_AST*>* dims_;
    int num_Dims_;
    int all_IntVals_;
    std::vector<std::string>* dims_Final_; // run-time filled in: array bounds'
    // final expression after evaluating the expressions in dims_ (as strings,
    // either an integer or a tmp variable), for reference later in visitor
};

// C99: doesn't exist (c. also comment to PreIncrIdExpr_AST)
class Assign_AST: public Stmt_AST{
public:
Assign_AST(IdExpr_AST* Id, Expr_AST* Expr)
    : Stmt_AST(Id, Expr)
    {
	setAddr(Id->Op().Lex());
	Id->Initialize();
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

class ModAssign_AST: public Assign_AST{
public:
ModAssign_AST(IdExpr_AST* Id, Expr_AST* Expr, token Type)
    : Assign_AST(Id, Expr), type_(Type)
    {
	setAddr(Id->Op().Lex());
	Id->Initialize();
	if (option_Debug)
	    std::cout << "\tcreated ModAssign_AST with LHS = " << addr_ << "\n";
    }

    token ModType(void) const { return type_; }

    void accept(AST_Visitor* Visitor)
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
	if (option_Debug) std::cout << "\tcreated an IfType...\n";
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

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }

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

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class For_AST: public Stmt_AST{
public:
For_AST(IterExprList_AST* Expr, Block_AST* Block)
    : Stmt_AST(Expr, Block)
    {
	if (option_Debug)
	    std::cout << "\tcreated a For_AST\n";
    }

    ~For_AST() {}

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

class While_AST: public For_AST{
public:
While_AST(IterExprList_AST* Expr, Block_AST* Block)
    : For_AST(Expr, Block)
    {
	if (option_Debug)
	    std::cout << "\tcreated While_AST\n";
    }

    void accept(AST_Visitor* Visitor) { Visitor->visit(this); }
};

#endif
