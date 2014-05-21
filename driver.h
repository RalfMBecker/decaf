/********************************************************************
* driver.h - header file for driver.cpp
*
********************************************************************/

#ifndef DRIVER_H_
#define DRIVER_H_

extern int no_lex_Errors;
extern int no_par_Errors;

std::ofstream* preProcess(std::string);
void initFrontEnd(std::string);
void collectParts(void);
void startParse(void);
void astToIR(void);
void cleanUp(void);

#endif
