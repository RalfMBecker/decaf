/********************************************************************
* visitor.cpp - implementation file for Visitor.h
*
********************************************************************/

#include "visitor.h"

int MakeIR_Visitor::count_Tmp_ = 0;
int MakeIR_Visitor::count_Lab_ = 0;

std::string MakeIR_Visitor::if_Next_ = "";
std::string MakeIR_Visitor::if_Done_ = "";
std::string MakeIR_Visitor::cond_First_ = "";
std::string MakeIR_Visitor::cond_Second_ = "";

std::vector<std::string> MakeIR_Visitor::active_Labels_ = 
    std::vector<std::string>();
