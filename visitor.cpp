/********************************************************************
* visitor.cpp - implementation file for Visitor.h
*
********************************************************************/

#include "visitor.h"

std::string MakeIR_Visitor::last_Tmp_ = "";

std::string MakeIR_Visitor::label_Break_ = "";
std::string MakeIR_Visitor::label_Cont_ = "";

int MakeIR_Visitor::needs_Label_ = 0;
std::vector<std::string> MakeIR_Visitor::active_Labels_ = 
    std::vector<std::string>();
