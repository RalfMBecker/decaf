/********************************************************************
* ir.cpp - Implementation file for ir.h
*
********************************************************************/

#include "ir.h"

ir_Rep iR_List;
ir_Rep iR_List_2; // nop's removed
ir_Rep iR_RtError_Targets;

// run-time error management 
std::vector<RtError_Type*> rtError_Table;
std::vector<Ds_Object*> Ds_Table;

std::vector<std::string>
appendLabels(std::vector<std::string> Old, std::vector<std::string> const& Add)
{
    std::vector<std::string> ret;
 
    ret = Old;
    std::vector<std::string>::const_iterator iter;
    for ( iter = Add.begin(); iter != Add.end(); iter++)
        ret.push_back(*iter);

    return ret;
}

ir_Rep
removeNOPs(ir_Rep const& Old)
{
    ir_Rep ir_New;
    ir_Rep::const_iterator iter;
    SSA_Entry* line;
    std::vector<std::string> labels;
    int Reset = 1;

    for ( iter = Old.begin(); iter != Old.end(); iter++){
	int is_Nop = ("nop" == (iter->second)->Op().Lex());
	int no_Lab = (iter->second)->Labels().empty();

	if ( !(is_Nop) ){
	    line = new SSA_Entry( *(iter->second) );
	    if ( !(labels.empty()) ){
		labels = appendLabels(line->Labels(), labels);  
		line->replaceLabels(labels);
	    }
	    labels.clear();
	    insertLine(line, ir_New, Reset);
	    Reset = 0;
	}
	else if ( is_Nop && !(no_Lab) ){
	    labels = appendLabels(labels, (iter->second)->Labels());  
	}
    }

    // print last if it was NOP
    iter--; 
    int is_Nop = ("nop" == (iter->second)->Op().Lex());
    if (is_Nop){
	line = new SSA_Entry( *(iter->second) );
	insertLine(line, ir_New, Reset);
    }

    return ir_New;
}

void 
insertLine(SSA_Entry* Line, ir_Rep& List, int Reset)
{
    static int current = 0;
    if (Reset) current = 0;

    List[++current]= Line;
}

void
printIR_List(ir_Rep const& List)
{
    ir_Rep::const_iterator iter;
    for (iter = List.begin(); iter != List.end(); iter++){
	std::cout.width(LINE);
	std::cout << iter->first << " ";
	(iter->second)->print();
	std::cout << "\n";
    }
}

// Run-time error management object
void
makeRtErrorTable(void)
{
    // private variables of the embedded class object (type Ds_Object*)
    std::string name, directive, value;
    directive = ".asciiz";
    // private variables of an RtError_Type*
    std::string label;
    Ds_Object* pDS;
    RtError_Type* pRT;

    // array bound checks: negative integer index
    name = "E_neg";
    value = "\"Error near %d: array bound negative (%s)\"";
    pDS = new Ds_Object(name, directive, value);
    Ds_Table.push_back(pDS);

    label = LABEL_ZERO_BOUND;
    pRT = new RtError_Type(label, pDS);
    rtError_Table.push_back(pRT);

    // array bound checks: requested larger than dimenstion
    name = "E_bound";
    value = "\"Error near %d: index out of bounds (%s)\"";
    pDS = new Ds_Object(name, directive, value);
    Ds_Table.push_back(pDS);

    label = LABEL_UPPER_BOUND;
    pRT = new RtError_Type(label, pDS);
    rtError_Table.push_back(pRT);
}

void
makeRtErrorTargetTable(ir_Rep& Target)
{
    std::vector<std::string> labels;
    std::string frame = "";
    std::string target, LHS, RHS;

    // some NOPs for visual clarity
    token op = token(tok_nop);
    SSA_Entry* line = new SSA_Entry(labels, op, target, LHS, RHS, frame); 
    insertLine(line, iR_RtError_Targets);
    insertLine(line, iR_RtError_Targets);

    // emit a syscall exit to not fall into this section
    op = token(tok_syscall);
    target = "exit";
    line = new SSA_Entry(labels, op, target, LHS, RHS, frame); 
    insertLine(line, iR_RtError_Targets);

    std::vector<RtError_Type*>::const_iterator iter;
    // each error readies its specific message before handing it on
    for ( iter = rtError_Table.begin(); iter != rtError_Table.end(); iter++){
	labels.push_back( (*iter)->Label() );
	op = token(tok_pushl);
	target = "$";
	target += (*iter)->Ds_Addr();
	line = new SSA_Entry(labels, op, target, LHS, RHS, frame);
	insertLine(line, iR_RtError_Targets);
	labels.clear();
	LHS = "";

	if ( (rtError_Table.back() != *iter) ){ 
	    op = token(tok_goto);
	    target = "L_eExit";
	    line = new SSA_Entry(labels, op, target, LHS, RHS, frame);
	    insertLine(line, iR_RtError_Targets);
	}
    }

    // common exit point
    op = token(tok_call);
    labels.push_back("L_eExit");
    target = "printf";
    line = new SSA_Entry(labels, op, target, LHS, RHS, frame);
    insertLine(line, iR_RtError_Targets);
    labels.clear();

    op = token(tok_syscall);
    target = "exit";
    line = new SSA_Entry(labels, op, target, LHS, RHS, frame);
    insertLine(line, iR_RtError_Targets);
}

void
printDataSection(void)
{
    std::cout << "---------------------------------------------------\n";
    // print header
    std::ostringstream tmp_Stream;
    tmp_Stream.width(10);
    tmp_Stream << ".section";
    tmp_Stream.width(8);
    tmp_Stream << ".data\n\n";
    std::cout << tmp_Stream.str();
    tmp_Stream.str("");

    // print global variables in data section
    std::vector<Ds_Object*>::const_iterator iter;
    for ( iter = Ds_Table.begin(); iter != Ds_Table.end(); iter++ )
	(*iter)->print();
    std::cout << "---------------------------------------------------\n\n";
}



