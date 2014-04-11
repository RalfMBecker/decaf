/********************************************************************
* tables.cpp - Tables for Decaf
*
*      bin_OpTable: table for Operator Precedence parsing of binary
*                   operator infix expressions
*      type_PrecTable: (basic) type precedence in coercions
*      type_WidthTable: width of types (bytes) on this machine
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
std::map<tokenType, int> bin_OpTable;
std::map<std::string, int> type_PrecTable;
std::map<std::string, int> type_WidthTable;
std::map<tokenType, int> logArithm_Table;
Env* root_Env;
Env* top_Env; // currently active environment table

// run-time globals
std::map<std::string, Symbol_Table> ST;

// static int used in Symbol Table maintenance in file tables.h
int Env::count_ = -1; // associate 0 with never-used root_Env pointer


// by making the values 0/1, can use simply in Op Precedence parsing
void
makeLogArithmTable(void)
{
    logArithm_Table[tok_log_or] = 0;  // is a logical operator, but not tracked
    logArithm_Table[tok_log_and] = 0; // per logic of Op Precedence parsing
    logArithm_Table[tok_log_eq] = 1;
    logArithm_Table[tok_log_ne] = 1;
    logArithm_Table[tok_lt] = 1;
    logArithm_Table[tok_le] = 1;
    logArithm_Table[tok_gt] = 1;
    logArithm_Table[tok_ge] = 1;
    logArithm_Table[tok_plus] = 0;
    logArithm_Table[tok_minus] = 0;
    logArithm_Table[tok_mult] = 0;
    logArithm_Table[tok_div] = 0;
    logArithm_Table[tok_mod] = 0;
}

int
isLogicalAdd(token t)
{
    if ( (logArithm_Table.end() != logArithm_Table.find(t.Tok())) )
	return logArithm_Table[t.Tok()];
    else
	return 0;
//	throw(Primary_Error(t.Lex(), "illegal in infix expression"));
}

// the following tokens have a precedence priority, but are not tracked
// using this table:
// tok_eq (=), tok_log_not (!), tok_minus (- unary), tok_sqopen ([),
// tok_dot (.)
void
makeBinOpTable(void)
{
    bin_OpTable[tok_log_or] = 100;
    bin_OpTable[tok_log_and] = 200;
    bin_OpTable[tok_log_eq] = 300;
    bin_OpTable[tok_log_ne] = 300;
    bin_OpTable[tok_lt] = 400;
    bin_OpTable[tok_le] = 400;
    bin_OpTable[tok_gt] = 400;
    bin_OpTable[tok_ge] = 400;
    bin_OpTable[tok_plus] = 500;
    bin_OpTable[tok_minus] = 500;
    bin_OpTable[tok_mult] = 600;
    bin_OpTable[tok_div] = 600;
    bin_OpTable[tok_mod] = 600;
}

int
opPriority(token t)
{
    if ( (bin_OpTable.end() != bin_OpTable.find(t.Tok())) )
	return bin_OpTable[t.Tok()];
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
    type_PrecTable["int"] = 10;
    type_PrecTable["double"] = 20;
}

int
typePriority(std::string const& Type)
{
    if ( (type_PrecTable.end() != type_PrecTable.find(Type)) )
	return type_PrecTable[Type];
    else
	return -1;
}

// type width in bytes
void
makeWidthTable(void)
{
    type_WidthTable["int"] = 4;
    type_WidthTable["double"] = 8;
}

int
typeWidth(std::string const& type)
{
    if ( (type_WidthTable.end() != type_WidthTable.find(type)) )
	return type_WidthTable[type];
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
    std::string Name = new_Id->Name();
    // add to Env* entry of Env ll rooted at root_Env
    if ( (0 == pEnv->findName(Name) ) ) // already in tables
	return -1;
    pEnv->insertName(Name, new_Id);

    // add into rt table ST, in the sub-table determined through 
    // the matching Env* pointer into the corresponding ct ll above
    std::string table_Name = pEnv->getTableName();
    std::map<std::string, Symbol_Table>::iterator iter;
    std::string Type(new_Id->Type());
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
    std::string name_Str(Id->Name());
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
	    std::cout << iter->first << "\t= " << (iter->second)->Type() << "\n";

	std::cout << "\n";
	p = p->getPrior();
    }
}

void
printSTInfo()
{
    std::map<std::string, Symbol_Table>::const_iterator iterOuter;
    for (iterOuter = ST.begin(); iterOuter != ST.end(); iterOuter++){
	std::cout << "Info for table " << iterOuter->first << "\n";
	std::cout << "---------------------------------------------------\n";
	Symbol_Table tmpST(iterOuter->second);
	std::map<std::string, Mem_Info> info(tmpST.getInfo());
	std::map<std::string, Mem_Info>::const_iterator iterInner;
	for (iterInner = info.begin(); iterInner != info.end(); iterInner++){
	    std::string name(iterInner->first);
	    std::cout << name;

	    std::cout << "\tType: " << tmpST.getType(name) << "\n";
	    std::cout << "\tMemType: " << tmpST.getMemType(name) << "\n";
	    std::cout << "\tOffset: " << tmpST.getOffset(name) << "\n";
	    std::cout << "\tWidth: " << tmpST.getWidth(name) << "\n";
	std::cout << "\n";
	}
    }

}
