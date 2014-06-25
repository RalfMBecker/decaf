/********************************************************************
* ir.h - IR for decaf
*
* Note: includes handling .data section, and emission or RT error
*       section 
*
********************************************************************/

#ifndef IR_H_
#define IR_H_

#include <sstream>
#include <ostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cstdio>

#include "lexer.h"

#define LABELS 16
#define SSA 10
#define ENV 8
#define LINE 5

#define LABEL_ZERO_BOUND "L_e0"
#define LABEL_UPPER_BOUND "L_e1"

// forward declaration
class Env;
class SSA_Entry;
class RtError_Type;
class Ds_Object;

typedef std::map<int, SSA_Entry*> ir_Rep;
// from ir.cpp
extern ir_Rep iR_List;
extern ir_Rep iR_List_2;
extern ir_Rep iR_RtError_Targets;

void insertLine(SSA_Entry*, ir_Rep&, int Reset = 0);
void printIR_List(ir_Rep const&);
ir_Rep removeNOPs(ir_Rep const&);

// also from ir.cpp
extern std::vector<RtError_Type*> rtError_Table;
extern std::vector<Ds_Object*> Ds_Table;

void makeRtErrorTable(void);
void makeRtErrorTargetTable(ir_Rep& Target);
void printDataSection(void);

class SSA_Entry{
public: 
SSA_Entry(std::vector<std::string> Labels, token Op, std::string Target, 
	  std::string LHS, std::string RHS, std::string Frame) 
    : labels_(Labels), op_(Op), target_(Target), lHS_(LHS), rHS_(RHS),
	frame_(Frame)
    { }   

    SSA_Entry(SSA_Entry const& r)
    {
	labels_ = r.Labels();
	op_ = r.Op();
	target_ = r.Target();
	lHS_ = r.LHS();
	rHS_ = r.RHS();
	frame_ = r.Frame();
    }

    void print() const
    {
	std::ostringstream tmp_Stream;
	std::string tmp_String;
	std::vector<std::string>::const_iterator iter;
	for (iter = labels_.begin(); iter != labels_.end(); iter++){
	    tmp_String += *iter;
	    tmp_String += ": ";
	}
	tmp_Stream.width(LABELS);
	tmp_Stream << tmp_String;

	tmp_Stream.width(SSA);
	tmp_Stream << op_.Lex();
	tmp_Stream << ":";
	tmp_Stream.width(SSA);
	tmp_Stream << target_;
	if ( ("" != lHS_) )
	    tmp_Stream << ",";
	else
	    tmp_Stream << " ";
	tmp_Stream.width(SSA);
	tmp_Stream << lHS_;
	if ( ("" != rHS_) )
	    tmp_Stream << ",";
	else
	    tmp_Stream << " ";
	tmp_Stream.width(SSA);
	tmp_Stream << rHS_;

	tmp_String = "(";
	tmp_String += frame_;
	tmp_String += ")";
	tmp_Stream.width(ENV);
	tmp_Stream << tmp_String;

	std::cout << tmp_Stream.str();
    }

    void addLabel(std::string Label) { labels_.push_back(Label); }
    void replaceLabels(std::vector<std::string> Labels) { labels_ = Labels; }

    std::vector<std::string> Labels() const { return labels_; }
    token Op(void) const { return op_; }
    std::string Target(void) const { return target_; }
    std::string LHS(void) const { return lHS_; }
    std::string RHS(void) const { return rHS_; }
    std::string Frame(void) const { return frame_;}

private:
    std::vector<std::string> labels_;
    token op_;
    std::string target_;
    std::string lHS_;
    std::string rHS_;
    std::string frame_;
};

// objects put into the data section of memory
class Ds_Object{
public:
Ds_Object(std::string N, std::string D, std::string V)
    : name_(N), directive_(D), value_(V) {}

    std::string Name(void) const { return name_; }
    std::string Directive(void) const { return directive_; }
    std::string Value(void) const { return value_; }

    void print() const
    {
	std::ostringstream tmp_Stream;
	std::string tmp_String;

	tmp_Stream << std::resetiosflags(std::ios::adjustfield);
	tmp_Stream << std::setiosflags(std::ios::left);
	tmp_String = name_;
	tmp_String += ":";
	tmp_Stream.width(10);
	tmp_Stream << tmp_String;

	tmp_Stream << std::resetiosflags(std::ios::adjustfield);
	tmp_Stream << std::setiosflags(std::ios::right);
	tmp_Stream.width(7);
	tmp_Stream << directive_;

	tmp_Stream.width(50);
	tmp_Stream << value_;
	tmp_Stream << "\n";

	std::cout << tmp_Stream.str();
    }

private:
    std::string name_;
    std::string directive_;
    std::string value_; // **TO DO: extend for general global initialized vars
};

// Run-time error management
class RtError_Type{
public:
RtError_Type(std::string L, Ds_Object* E)
    : label_(L), error_(E) {}

    std::string Label(void) const { return label_; }
    std::string Ds_Addr(void) const { return error_->Name(); }

private:
    std::string label_;
    Ds_Object* error_;
};

#endif
