/********************************************************************
* ast.h - implementation file for ast of decaf
*
********************************************************************/

#include "ast.h"
#include "lexer.h"

// implement static non-const class variables
int NodeAST::label_Count_ = 0;
int TempAST::count_ = 0;
