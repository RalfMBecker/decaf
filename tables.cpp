/********************************************************************
* tables.cpp - Tables for Decaf
*
*      bin_OpTable: table for Operator Precedence parsing of binary
*                   operator infix expressions
*      type_PrecTable: (basic) type precedence in coercions
*      type_WidthTable: width of types (bytes) on this machine
*      root_Env: root of a (one-sided) linked list of compile-time
*                symbol tables (linking back, to enclosing scope)
*      ST: run-time symbol table (for use of backend)
*
********************************************************************/

#include <map>
#include <string>
#include "lexer.h"
#include "tables.h"

// compile-time globals
std::map<tokenType, int> bin_OpTable;
std::map<std::string, int> type_PrecTable;
std::map<std::string, int> type_WidthTable;
Env* root_Env;

// run-time globals
std::map<std::string, symbolTable> ST;

// static int used in Symbol Table maintenance in file tables.h
int Env::count_ = 0;

void
makeBinOpTable(void)
{
    bin_OpTable[tok_eq]= 100;
    bin_OpTable[tok_log_or] = 200;
    bin_OpTable[tok_log_and] = 300;
    bin_OpTable[tok_log_eq] = 400;
    bin_OpTable[tok_log_ne] = 400;
    bin_OpTable[tok_lt] = 500;
    bin_OpTable[tok_le] = 500;
    bin_OpTable[tok_gt] = 500;
    bin_OpTable[tok_ge] = 500;
    bin_OpTable[tok_plus] = 600;
    bin_OpTable[tok_minus] = 600;
    bin_OpTable[tok_mult] = 700;
    bin_OpTable[tok_div] = 700;
    bin_OpTable[tok_mod] = 700;
    bin_OpTable[tok_log_not] = 800;
    bin_OpTable[tok_minus] = 800;
    bin_OpTable[tok_sqopen] = 900;
    bin_OpTable[tok_dot] = 900;
}

// Prepare type_Table with basic types and their coercion priority
// (void is not a legal type for var declaration).
// As we'll also use the table for a 'valid type' check, we added string
// (which cannot be converted to any other type)
void
makeTypePrecTable(void)
{
    type_PrecTable["string"] = 0;
    type_PrecTable["bool"] = 10;
    type_PrecTable["integer"] = 20;
    type_PrecTable["double"] = 30;
}

// relocate as method of IDs & Arrays
int
typePriority(std::string type)
{
    if ( (type_PrecTable.end() != type_PrecTable.find(type)) )
	return type_PrecTable[type];
    else
	return -1;
}

// type width in bytes
void
makeWidthTable(void)
{
    type_PrecTable["bool"] = 4;
    type_PrecTable["integer"] = 4;
    type_PrecTable["double"] = 8;
}

// relocate as method of IDs & Arrays & classes
int
typeWidth(std::string type)
{
    if ( (type_WidthTable.end() != type_WidthTable.find(type)) )
	return type_WidthTable[type];
    else
	return -1;
}

Env* 
makeEnvRoot(void)
{
    return (root_Env = new Env(0));
}

// Builder to maintain parallel compile-time and run-time info about 
// variables in a scope.
Env*
addEnv(Env* Prior)
{
    Env* pNew_Env = new Env(Prior);
    std::string new_Name = pNew_Env->getTableName();
    symbolTable newST = symbolTable(new_Name);
    ST[new_Name] = newST;

    return pNew_Env;
}


// pEnv will be used during compile-time, so go via this ll
int
addEnvName(Env* pEnv, std::string new_Name, std::string Type, 
	   std::string Mem, int Offset, int Width)
{
    // add to Env* entry of Env ll rooted at root_Env
    if ( (0 == pEnv->findName(new_Name) ) ) // already in tables
	return -1;
    pEnv->insertName(new_Name, Type);

    // add into rt table ST, in the sub-table determined through 
    // the matching Env* pointer into the corresponding ct ll above
    std::string table_Name = pEnv->getTableName();
    std::map<std::string, symbolTable>::iterator iter;
    if ( (ST.end() == (iter = ST.find(table_Name))) )
	return -1;
    symbolTable s(iter->second);
    s.insertName(new_Name, Type, Mem, Offset, Width);

    return 0;
}


