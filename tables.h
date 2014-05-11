/********************************************************************
* tables.h - header file for tables.cpp
*            Also defines tables and type classes
*
********************************************************************/

#ifndef TABLES_H_
#define TABLES_H_

#include <map>
#include <string>
#include <sstream>

#include "lexer.h"
#include "ast.h"

void errExit(int, const char* format, ...);

// forward declarations
class Expr_AST;
class IdExpr_AST;

// compile time globals
extern std::map<tokenType, int> binOp_Table;
extern std::map<std::string, int> typePrec_Table;
extern std::map<std::string, int> typeWidth_Table;

void makeBinOpTable(void);
void makeTypePrecTable(void);
void makeWidthTable(void);
void addToWidthTable(std::string Name, int Width);
int typePriority(std::string const&);
int typeWidth(std::string const&);
int opPriority(token t);

// compile time static declaration check
class Env;
extern Env* root_Env;
extern Env* top_Env;

Env* makeEnvRootTop(void);
Env* addEnv(Env*);
void printEnvAncestorInfo(Env*);

// ct basic type & arrays of basic type management
int addVarDeclToEnv(Env* pEnv, VarDecl_AST* new_Object, std::string MemType);
VarDecl_AST* findVarByIdId(Env* p, IdExpr_AST* Id);
VarDecl_AST* findVarByName(Env* p, std::string Name);
Env* findVarFrame(Env* p, std::string Name);

// runtime globals
class Symbol_Table;
extern std::map<std::string, Symbol_Table> ST;
void printSTInfo(void);

// Table for: basic types; arrays of basic types
// Compile-time object, part of a linked list of (ct) Symbol Tables, each
// a (name, <basic type>/class) pair
// Handling 'name' here isn't the most logical, but will do (name=table name).
// Name is needed to maintain the run-time version of the STs.
class Env{
public:
    Env(Env* P = 0)
	: prior_(P)
    { 
	std::stringstream tmp;
	tmp << "Env" << ++count_;
	name_ = tmp.str();
	runtime_StackAdj_ = std::vector<std::string>();
    }

    Env* getPrior(void) const { return prior_; }
    std::string getTableName(void) const { return name_; }
    std::map<std::string, VarDecl_AST*> getType(void) const { return type_; } 

    void addAdj(std::string New_Adj) { runtime_StackAdj_.push_back(New_Adj); }
    std::vector<std::string> getAdj(void) const { return runtime_StackAdj_; }

    int findName(std::string entry_Name)
    {
	if ( (type_.end() == type_.find(entry_Name)) )
	    return -1;
	else
	    return 0;
    }

    Env& insertName(std::string new_Name, VarDecl_AST* t)
    {
	if ( (-1 == findName(new_Name)) )
	    type_[new_Name] = t;
	return *this;
    }

    VarDecl_AST* readName(std::string search_Name)
    {
	if ( (-1 == findName(search_Name)) )
	    return 0;
	else
	    return type_[search_Name];
    }

private:
    static int count_;
    std::string name_;
    Env* prior_;
    std::map<std::string, VarDecl_AST*> type_;
    std::vector<std::string> runtime_StackAdj_; // for variable length arrays

};

// Run-time symbol table information
class Mem_Info{
public:
    Mem_Info(std::string Type = "", std::string memT = "", int Offset = 0, 
	    int Width = 0)
	: type_(Type), memType_(memT), offset_(Offset), width_(Width) {}

    std::string Type(void) const { return type_; }
    std::string MemType(void) const { return memType_; }
    int Offset(void) const { return offset_; }
    int Width(void) const { return width_; }

private:
    std::string type_; // basic; class
    std::string memType_; // stack; heap
    int offset_; // rel. offset to mem areas reserved by Env
    int width_; // size of object
};


// Run-time object to manage storage allocation
// Name created by the corresponding compile-time object, and fed
// from maker function addItToEnv(). 
// Offset: rel offset to beginning of mem area that will be reserved 
//         for objects in the scope managed by this Symbol_Table, by
//         heap and stack area
class Symbol_Table{
public:
    Symbol_Table(std::string Name = "")
	: name_(Name) { offsetHeap_ = offsetStack_ = 0; }

    Symbol_Table(const Symbol_Table& s)
    {
	offsetHeap_ = s.getOffsetHeap();
	offsetStack_ = s.getOffsetStack();
	name_ = s.getName();
	info_ = s.getInfo();
    }

    int getOffsetHeap(void) const { return offsetHeap_; }
    int getOffsetStack(void) const { return offsetStack_; }
    std::string getName(void) const { return name_; }
    std::map<std::string, Mem_Info> getInfo(void) const { return info_; }

    int findName(std::string search_Name)
    {
	if ( (info_.end() == info_.find(search_Name)) )
	    return -1;
	else
	    return 0;
    }

    // Note: function overloaded
    Symbol_Table& insertName(std::string new_Name, Mem_Info i)
    {
	if ( (-1 == findName(new_Name)) )
	    info_[new_Name] = i;
	return *this;
    }

    Symbol_Table& insertName(std::string new_Name, std::string Type, 
			    std::string Mem, int Width)
    {
	int tmp;
	if ( ("heap" == Mem) ){
	    tmp = offsetHeap_;
	    offsetHeap_ += Width;
	}
	else if ( ("stack" == Mem) ){
	    tmp = offsetStack_;
	    offsetStack_ += Width;
	}
	else  
	    errExit(0, "invalid use of Symbol_Table (abort)\n");
	Mem_Info tmpInfo(Type, Mem, tmp, Width);
	insertName(new_Name, tmpInfo);
	return *this;
    }

    Mem_Info readNameInfo(std::string read_Name)
    {
	if ( (-1 == findName(read_Name)) )
	    return Mem_Info(0);
	else
	    return info_[read_Name];
    }

    // retrieving Mem_Info fields ("" == 'not defined')
    std::string const getType(std::string elem_Name)
    {
	Mem_Info tmp(readNameInfo(elem_Name));
	return tmp.Type();
    }

    std::string const getMemType(std::string elem_Name)
    {
	Mem_Info tmp(readNameInfo(elem_Name));
	return tmp.MemType();
    }

    int const getOffset(std::string elem_Name)
    {
	Mem_Info tmp(readNameInfo(elem_Name));
	return tmp.Offset();
    }

    int const getWidth(std::string elem_Name)
    {
	Mem_Info tmp(readNameInfo(elem_Name));
	return tmp.Width();
    }

private:
    int offsetHeap_;
    int offsetStack_;
    std::string name_;
    std::map<std::string, Mem_Info> info_;    
};

#endif


