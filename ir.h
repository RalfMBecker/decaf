/********************************************************************
* ir.h - IR for decaf
*
********************************************************************/

#ifndef IR_H_
#define IR_H_

#include <sstream>
#include <ostream>
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

// forward declaration
class Env;
class SSA_Entry;

typedef std::map<int, SSA_Entry*> ir_Rep;
// from ir.cpp
extern ir_Rep iR_List;
extern ir_Rep iR_List_2;

void insertLine(SSA_Entry*, ir_Rep&, int Reset = 0);
void printIR_List(ir_Rep const&);
ir_Rep removeNOPs(ir_Rep const&);

class SSA_Entry{
public: 
SSA_Entry(std::vector<std::string> Labels, token Op, std::string Target, 
	  std::string LHS, std::string RHS, std::string Frame)
    : labels_(Labels),op_(Op),target_(Target),lHS_(LHS),rHS_(RHS),frame_(Frame)
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

#endif
