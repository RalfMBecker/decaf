/********************************************************************
* IR.cpp - Implementation file for IR.h
*
********************************************************************/

#include "IR.h"

std::map<int, IR_Line*> iR_List;

void 
insertLine(IR_Line* Line)
{
    static int current = 0;
    iR_List[++current]= Line;
}

void
printIR_List(void)
{
    std::map<int, IR_Line*>::const_iterator iter;
    for (iter = iR_List.begin(); iter != iR_List.end(); iter++){
	std::cout.width(LINE);
	std::cout << iter->first << " ";
	(iter->second)->print();
	std::cout << "\n";
    }
}

