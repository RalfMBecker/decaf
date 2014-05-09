/********************************************************************
* visitor.cpp - implementation file for Visitor.h
*
********************************************************************/

#include "visitor.h"

int MakeIR_Visitor::count_Tmp_ = 0;
int MakeIR_Visitor::count_Lab_ = 0;

std::string MakeIR_Visitor::label_Break_ = "";
std::string MakeIR_Visitor::label_Cont_ = "";

int MakeIR_Visitor::needs_Label_ = 0;
std::vector<std::string> MakeIR_Visitor::active_Labels_ = 
    std::vector<std::string>();
