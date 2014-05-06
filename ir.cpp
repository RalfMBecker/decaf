/********************************************************************
* ir.cpp - Implementation file for ir.h
*
********************************************************************/

#include "ir.h"

ir_Rep iR_List;
ir_Rep iR_List_2; // nop's removed

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
