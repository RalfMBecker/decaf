/********************************************************************
* tables.cpp - Tables for Decaf
*
*             bin_OpTable: table for Operator Precedence parsing of
*                         binary operator infix expressions
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
extern std::map<std::string, symbolTable> STs;

// static int used in Symbol Table maintenance in file tables.h
int Env::count_ = 0;
int offsetHeap_ = 0;
int offsetStack_ = 0;

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
