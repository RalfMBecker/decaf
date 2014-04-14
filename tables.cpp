/********************************************************************
* tables.cpp - Tables for Decaf
*
*      binOP_Table: table for Operator Precedence parsing of binary
*                   operator infix expressions
*      typePrec_Table: (basic) type precedence in coercions
*      typeWidth_Table: width of types (bytes) on this machine
*      Env*: linked list of compile-time frames
*      root_Env: root of a (one-sided) linked list of compile-time
*                symbol tables (linking back, to enclosing scope)
*      top_Env: pointer to current Activation Block
*      ST: run-time symbol table (for use of backend)
*
********************************************************************/

#include <map>
#include <string>

#include "lexer.h"
#include "error.h"
#include "tables.h"

// forward declarations
class IdExpr_AST;

// compile-time globals
std::map<tokenType, int> binOP_Table;
std::map<std::string, int> typePrec_Table;
std::map<std::string, int> typeWidth_Table;
Env* root_Env;
Env* top_Env; // currently active environment table

// run-time globals
std::map<std::string, Symbol_Table> ST;

// static int used in Symbol Table maintenance in file tables.h
int Env::count_ = -1; // associate 0 with never-used root_Env pointer

// the following tokens have a precedence priority, but are not tracked
// using this table:
// tok_eq (=), tok_log_not (!), tok_minus (- unary), tok_sqopen ([),
// tok_dot (.)
void
makeBinOpTable(void)
{
    binOP_Table[tok_log_or] = 100;
    binOP_Table[tok_log_and] = 200;
    binOP_Table[tok_log_eq] = 300;
    binOP_Table[tok_log_ne] = 300;
    binOP_Table[tok_lt] = 400;
    binOP_Table[tok_le] = 400;
    binOP_Table[tok_gt] = 400;
    binOP_Table[tok_ge] = 400;
    binOP_Table[tok_plus] = 500;
    binOP_Table[tok_minus] = 500;
    binOP_Table[tok_mult] = 600;
    binOP_Table[tok_div] = 600;
    binOP_Table[tok_mod] = 600;
}

int
opPriority(token t)
{
    if ( (binOP_Table.end() != binOP_Table.find(t.Tok())) )
	return binOP_Table[t.Tok()];
    else // this will stop OpPrecedence parsing once we hit a
	return -1;  // non-op while evaluating InfixEpxr
}

// Prepare type_Table with basic types and their coercion priority
// (void is not a legal type for var declaration).
// As we'll also use the table for a 'valid type' check, we added string
// (which cannot be converted to any other type)
void
makeTypePrecTable(void)
{
    typePrec_Table["int"] = 10;
    typePrec_Table["double"] = 20;
}

int
typePriority(std::string const& Type)
{
    if ( (typePrec_Table.end() != typePrec_Table.find(Type)) )
	return typePrec_Table[Type];
    else
	return -1;
}

// type width in bytes
void
makeWidthTable(void)
{
    typeWidth_Table["int"] = 4;
    typeWidth_Table["double"] = 8;
}

// add class width in bytes, as needed (no shadowing assumed)
void
addToWidthTable(std::string Name, int Width)
{
    if ( (typeWidth_Table.end() == typeWidth_Table.find(Name)) )
	typeWidth_Table[Name] = Width;
    else
	errExit(0, "shadowing class definitions not allowed");
}

int
typeWidth(std::string const& Type)
{
    if ( (typeWidth_Table.end() != typeWidth_Table.find(Type)) )
	return typeWidth_Table[Type];
    else
	return -1;
}

Env* 
makeEnvRootTop(void)
{
    return ( (top_Env = root_Env = new Env(0)) );
}

// Builder to maintain parallel compile-time and run-time info about 
// variables in a scope (new link in ct ll rooted at root_Env; new entry
// into rt map ST)
// ***TO DO: add delete() method to visitor (easiest probably if we
//           add an Env* env_ object somewhere (Block_AST? Node_AST?)
Env*
addEnv(Env* Prior)
{
    Env* pNew_Env = new Env(Prior);
    std::string new_Name = pNew_Env->getTableName();
    Symbol_Table newST = Symbol_Table(new_Name);
    ST[new_Name] = newST;

    return ( (top_Env = pNew_Env) );
}

// pEnv will be used during compile-time, so go via this ll
// ***** TO DO: ADJUST TO DIFFERENT TYPE WHEN CLASSES/FUNCTIONS ADDED *****
int
addIdToEnv(Env* pEnv, IdExpr_AST* new_Id, std::string MemType)
{
    std::string Name = new_Id->Addr();
    // add to Env* entry of Env ll rooted at root_Env
    if ( (0 == pEnv->findName(Name) ) ) // already in tables
	return -1;
    pEnv->insertName(Name, new_Id);

    // add into rt table ST, in the sub-table determined through 
    // the matching Env* pointer into the corresponding ct ll above
    std::string table_Name = pEnv->getTableName();
    std::map<std::string, Symbol_Table>::iterator iter;
    std::string Type(new_Id->Type().Lex());
    int Width(new_Id->TypeW());

    if ( (ST.end() == (iter = ST.find(table_Name))) )
	return -2;
    (iter->second).insertName(Name, Type, MemType, Width);

    return 0;
}

Expr_AST*
findNameInHierarchy(Env* p, std::string Name)
{
    while ( (root_Env != p) ){
	std::map<std::string, Expr_AST*>::const_iterator iter; 
	std::map<std::string, Expr_AST*> tmp_Type = p->getType();
	for (iter = tmp_Type.begin(); iter != tmp_Type.end(); iter++)
	    if ( (Name == iter->first) )
		return iter->second;
	p = p->getPrior();
    }
    return 0;
}

Expr_AST*
findIdInHierarchy(Env* p, IdExpr_AST* Id)
{
    std::string name_Str(Id->Addr());
    return findNameInHierarchy(p, name_Str);
}

// debugging functions
void
printEnvAncestorInfo(Env* p)
{
    while ( (root_Env != p) ){
	std::cout << "Info for table " << p->getTableName() << "\n";
	std::cout << "-----------------------------------\n";
	std::map<std::string, Expr_AST*>::const_iterator iter; 
	std::map<std::string, Expr_AST*> tmp_Type = p->getType();
	for (iter = tmp_Type.begin(); iter != tmp_Type.end(); iter++)
	    std::cout << iter->first << "\t= " << (iter->second)->Type().Lex() 
		      << "\n";

	std::cout << "\n";
	p = p->getPrior();
    }
}

void
printSTInfo()
{
    std::map<std::string, Symbol_Table>::const_iterator iter_Outer;
    for (iter_Outer = ST.begin(); iter_Outer != ST.end(); iter_Outer++){
	std::cout << "Info for table " << iter_Outer->first << "\n";
	std::cout << "---------------------------------------------------\n";
	Symbol_Table tmpST(iter_Outer->second);
	std::map<std::string, Mem_Info> info(tmpST.getInfo());
	std::map<std::string, Mem_Info>::const_iterator iter_Inner;
	for (iter_Inner = info.begin(); iter_Inner != info.end(); iter_Inner++){
	    std::string name(iter_Inner->first);
	    std::cout << name;

	    std::cout << "\tType: " << tmpST.getType(name) << "\n";
	    std::cout << "\tMemType: " << tmpST.getMemType(name) << "\n";
	    std::cout << "\tOffset: " << tmpST.getOffset(name) << "\n";
	    std::cout << "\tWidth: " << tmpST.getWidth(name) << "\n";
	std::cout << "\n";
	}
    }

}
