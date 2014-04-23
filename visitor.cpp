/********************************************************************
* visitor.cpp - implementation file for Visitor.h
*
********************************************************************/

#include <vector>
#include <string>
#include "visitor.h"

int MakeIR_Visitor::count_Tmp_ = 0;
int MakeIR_Visitor::count_Lab_ = 0;
std::string MakeIR_Visitor::if_Next_ = "";
std::string MakeIR_Visitor::if_Done_ = "";
