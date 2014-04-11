/********************************************************************
* driver.h - header file for driver.cpp
*
********************************************************************/

extern int no_lex_Errors;
extern int no_par_Errors;

void panicModeFwd(void);
void initFrontEnd(std::string);
void collectParts(void);
void startParse(void);
