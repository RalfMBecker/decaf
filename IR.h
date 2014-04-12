/********************************************************************
* IR.h - IR for decaf
*
********************************************************************/

#include <string>
#include <vector>
#include <iostream>
#include <cstdio>

#include "lexer.h"
#include "tables.h"

// assume for now short variable names; worry about properness later
class SSA_Entry{
public: 
    SSA_Entry(token Op, std::string Target, std::string LHS, std::string RHS)
	:op_(Op), target_(Target), lHS_(LHS), rHS_(RHS) {}

    void print() const
    { // will adjust after reviewing C++ formating **TO DO**
	printf("%5s: %5s, %5s, %5s\n", op_.Lex().c_str(), 
	       target_.c_str(), lHS_.c_str(), rHS_.c_str());
    }

private:
    token op_;
    std::string target_;
    std::string lHS_;
    std::string rHS_;
};

class IR_Entry{
public:
IR_Entry(std::vector<std::string> Vec, std::string Frame, SSA_Entry SSA)
    : label_List_(Vec), frame_(Frame), sSA_(SSA) {}

    void print() const
    {
	std::vector<std::string>::const_iterator iter;
	for (iter = label_List_.begin(); iter != label_List_.end(); iter++)
	    std::cout << *iter << ":";
	std::cout << "\t"; // make this better...***TO DO****
        printf("%5s - ", frame_.c_str());
	sSA_.print();
    }

private:
    std::vector<std::string> label_List_;
    std::string frame_;
    SSA_Entry sSA_;
};

class IR_Program{
public:
    IR_Program(std::map<int, IR_Entry> List)
	: programIR_List_(List) {}

    void print() const 
    {
	std::map<int, IR_Entry>::const_iterator iter;
	for (iter = programIR_List_.begin(); iter != programIR_List_.end(); 
	     iter++);
	std::cout << iter->first << ":"; // needs formatting ***TO DO***
	(iter->second).print();
    }

private:
    std::map<int, IR_Entry> programIR_List_;
};
