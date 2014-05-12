/********************************************************************
* ir.cpp - Implementation file for ir.h
*
********************************************************************/

#include "ir.h"

ir_Rep iR_List;
ir_Rep iR_List_2; // nop's removed
ir_Rep iR_RtError_Targets;

// run-time error management 
std::map<int, RtError_Type*> rtError_Table;
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

// run-time error management object
void
makeRtErrorTable(void)
{
    // array bound checks: negative integer index
    std::string label = "L_e0";
    std::string msg = "array bound negative";
    std::string dsA = "E_neg";
    RtError_Type* pRT = new RtError_Type(label, msg, dsA);
    rtError_Table[0] = pRT;

    // array bound checks: requested larger than dimenstion
    label = "L_e1";
    msg = "index out of bounds";
    dsA = "E_bounds";
    pRT = new RtError_Type(label, msg, dsA);
    rtError_Table[1] = pRT;
}

// TO DO: conditionally do all this after switching on global, indicating
//        a dynamic array has been declared
void
makeRtErrorTargetTable(ir_Rep& Target)
{
    std::map<int, RtError_Type*>::const_iterator iter;

    std::vector<std::string> labels;
    std::string frame = "";
    token op = token(tok_nop);
    std::string target, LHS, RHS;
    SSA_Entry* line = new SSA_Entry(labels, op, target, LHS, RHS, frame); 
    insertLine(line, iR_RtError_Targets);
    insertLine(line, iR_RtError_Targets);

    op = token(tok_syscall);
    for ( iter = rtError_Table.begin(); iter != rtError_Table.end(); iter++){
	labels.push_back( (iter->second)->Label() );
	target = "printf";
	line = new SSA_Entry(labels, op, target, LHS, RHS, frame);
	insertLine(line, iR_RtError_Targets);
	labels.clear();

	target = "exit";
	line = new SSA_Entry(labels, op, target, LHS, RHS, frame);
	insertLine(line, iR_RtError_Targets);	
    }
}



// std::map<int, RtError_Type*> rtError_Table;
//insertLine(SSA_Entry* Line, ir_Rep& List, int Reset)
//void
//printRtErrorTable(void){
//ir_Rep iR_RtError_Targets;



//std::vector<Ds_Object> Ds_Table;
//Ds_Object(std::string N, std::string D, std::string V)

