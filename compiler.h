#ifndef COMPILER_H_
#define COMPILER_H_

#include <fstream>     // file streams and operations
#include <iostream>    // cin, cout, cerr, etc.
#include <cstdlib>     // exit(), etc.
#include <errno.h>      // errno()

#include <vector>
#include <map>
//#include <string>

#include "error.h"
//#include <cstdlib> // for exit, EXIT_SUCCESS, EXIT_FAILURE, etc.

#define MAX_ID 31
#define MAX_LIT 32
#define MAX_STR 32

/*
extern std::map<char, int> BinopPrecedence;

void BinopPrecedenceCreate();
// make available to driver
int getNextToken();
ExprAST* ParseExpression();
FunctionAST* ParseDefinition(void);
PrototypeAST* ParseExtern(void);
FunctionAST* ParseTopLevelExpr(void);

extern int CurTok;
*/

#endif
