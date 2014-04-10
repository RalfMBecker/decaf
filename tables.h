/********************************************************************
* tables.h - header file for tables.cpp
*
********************************************************************/

#ifndef TABLES_H_
#define TABLES_H_

#include <map>
#include <string>
#include <sstream>

#include "ast.h"

// compile time globals
extern std::map<tokenType, int> bin_OpTable;
extern std::map<std::string, int> type_PrecTable;
extern std::map<std::string, int> type_WidthTable;

class Env;
extern Env* root_Env;
extern Env* top_Env;

Env* makeEnvRootTop(void);
Env* addEnv(Env*);
int addIdToEnv(Env* pEnv, IdExprAST* new_Object, std::string MemType);
ExprAST* findIdInHierarchy(Env* p, IdExprAST* Id);
ExprAST* findNameInHierarchy(Env* p, std::string Name);
void printEnvAncestorInfo(Env*);

// runtime global
class symbolTable;
extern std::map<std::string, symbolTable> ST;
void printSTInfo(void);

// other table helpers
void makeBinOpTable(void);
void makeTypePrecTable(void);
void makeWidthTable(void);
int typePriority(std::string const&);
int typeWidth(std::string const&);

// Compile-time object, part of a linked list of (ct) Symbol Tables, each
// a (name, <basic type>/class) pair
// Handling 'name' here isn't the most logical, but will do (name=table name).
// Name is needed to maintain the run-time version of the STs.
class Env{
public:
    Env(Env* P = 0)
	: prior_(P)
    { 
	count_++; 
	std::stringstream tmp;
	tmp << "Env" << count_;
	name_ = tmp.str();
    }

    Env* getPrior(void) const { return prior_; }
    std::string getTableName(void) const { return name_; }
    std::map<std::string, ExprAST*> getType(void) const { return type_; } 

    int findName(std::string entry_Name)
    {
	if ( (type_.end() == type_.find(entry_Name)) )
	    return -1;
	else
	    return 0;
    }

    Env& insertName(std::string new_Name, ExprAST* t)
    {
	if ( (-1 == findName(new_Name)) )
	    type_[new_Name] = t;
	return *this;
    }

    ExprAST* readName(std::string search_Name)
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
    std::map<std::string, ExprAST*> type_; // <basic type>/class    
};

// Run-time symbol table information
class memInfo{
public:
    memInfo(std::string Type = "", std::string memT = "", int Offset = 0, 
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
// from builder function ************ENTER NAME***************
// Offset: rel offset to beginning of mem area that will be reserved 
//         for objects in the scope managed by this symbolTable, by
//         heap and stack area
class symbolTable{
public:
    symbolTable(std::string Name = "")
	: name_(Name) { offsetHeap_ = offsetStack_ = 0; }

    symbolTable(const symbolTable& s)
    {
	offsetHeap_ = s.getOffsetHeap();
	offsetStack_ = s.getOffsetStack();
	name_ = s.getName();
	info_ = s.getInfo();
    }

    int getOffsetHeap(void) const { return offsetHeap_; }
    int getOffsetStack(void) const { return offsetStack_; }
    std::string getName(void) const { return name_; }
    std::map<std::string, memInfo> getInfo(void) const { return info_; }

    int findName(std::string search_Name)
    {
	if ( (info_.end() == info_.find(search_Name)) )
	    return -1;
	else
	    return 0;
    }

    // Note: function overloaded
    symbolTable& insertName(std::string new_Name, memInfo i)
    {
	if ( (-1 == findName(new_Name)) )
	    info_[new_Name] = i;
	return *this;
    }

    symbolTable& insertName(std::string new_Name, std::string Type, 
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
	else  // little point singling out this as the only validity check
	    ; // as function only used by compiler writer
	memInfo tmpInfo(Type, Mem, tmp, Width);
	insertName(new_Name, tmpInfo);
	return *this;
    }

    memInfo readNameInfo(std::string read_Name)
    {
	if ( (-1 == findName(read_Name)) )
	    return memInfo(0);
	else
	    return info_[read_Name];
    }

    // retrieving memInfo fields ("" == 'not defined')
    std::string const getType(std::string elem_Name)
    {
	memInfo tmp(readNameInfo(elem_Name));
	return tmp.Type();
    }

    std::string const getMemType(std::string elem_Name)
    {
	memInfo tmp(readNameInfo(elem_Name));
	return tmp.MemType();
    }

    int const getOffset(std::string elem_Name)
    {
	memInfo tmp(readNameInfo(elem_Name));
	return tmp.Offset();
    }

    int const getWidth(std::string elem_Name)
    {
	memInfo tmp(readNameInfo(elem_Name));
	return tmp.Width();
    }


private:
    int offsetHeap_;
    int offsetStack_;
    std::string name_;
    std::map<std::string, memInfo> info_;    
};

#endif


