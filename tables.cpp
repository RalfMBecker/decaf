/********************************************************************
* tables.cpp - Tables for Decaf
*
*             bin_OpTable: table for Operator Precedence parsing of
*                         binary operator infix expressions
*
********************************************************************/

// **********MUST CREATE TYPE OBJECT (as user-declared types (classes)
//           are legal, only way to check) **************************
// (void is not a legal type for var declaration)

#include <map>
#include <string>
#include "lexer.h"

std::map<tokenType, int> bin_OpTable;
std::map<std::string, std::string> type_Table;

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

// prepare type_Table with basic types. User-defined types might be added.
void
makeTypeTable()
{
    type_Table["integer"] = "basic";
    type_Table["double"] = "basic";
    type_Table["bool"] = "basic";
    type_Table["string"] = "basic";
}


