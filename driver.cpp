/********************************************************************
* driver.cpp - driver for Decaf
*
********************************************************************/

// include max. # of #include files to monitor dependencies
#include <fstream>
#include <cstdio>
#include <string.h> // for basename()
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"
#include "parser.h"
#include "ir.h" 
#include "visitor.h"

void preProcess(std::string);

extern Node_AST* pFirst_Node; // double declaration (from ast.h) - for clarity

extern int no_lex_Errors;
extern int no_par_Errors;
extern int emitRtError_Section;

int option_Debug = 0;
int option_Preproc = 0;  // pre-process, create file, and exit
int option_IR = 0;       // create IR, create IR file, and exit
int option_OptLevel = 0; // 0 - remove NOPs

std::string base_Name;
std::fstream* input; // first source file, then preproc'ed file
std::fstream* file_Source; // as we change input*, to be able to delete
                            // pointer to source file at end
std::fstream* file_Preproc;
std::fstream* file_IR;

void
deallocateAST(Node_AST* P)
{
    if ( (0 == P) ) 
	return;
 
    if ( (0 != P->LChild()) )
	deallocateAST(P->LChild());
    if ( (0 != P->RChild()) )
	deallocateAST(P->RChild());

    if (0 != P){
	if ( (1 < P->RefCount()) )
	    P->RefCountMinus();
	else
	    delete P;
    }
}

// initial call: root_Env
void
deallocateEnv(Env* P)
{ 
    std::vector<Env*>::iterator iter;
    std::vector<Env*> c = P->Children();
    if ( !(c.empty()) ){
	for ( iter = c.begin(); iter != c.end(); iter++ )
	    deallocateEnv(*iter);
    }
    delete P;
}

void
deallocateIR(void)
{
    if ( !(rtError_Table.empty()) ){
	std::vector<RtError_Type*>::iterator iter;
	std::vector<RtError_Type*> t = rtError_Table;	
	for ( iter = t.begin(); iter != t.end(); iter++ )
	    if ( (0 != *iter) ) delete *iter;
    }

    if ( !(Ds_Table.empty()) ){
	std::vector<Ds_Object*>::iterator iter;
	std::vector<Ds_Object*> t = Ds_Table;
	for ( iter = t.begin(); iter != t.end(); iter++ )
	    if ( (0 != *iter) ) delete *iter;
    }
}

void
deallocate(Node_AST* P)
{
    deallocateAST(P);
    deallocateEnv(root_Env);
    deallocateIR();
}

// input* is just a global pointer that will be destroyed automatically at
// program end; all memory areas on the heap are released
void
cleanUp(void)
{
    deallocate(pFirst_Node);

    delete file_Source;
    delete file_Preproc;

    if ( !(option_Preproc) ){
	std::string tmp_Str = base_Name + ".pre";
	unlink(tmp_Str.c_str());
    }
    if (option_IR)
	delete file_IR;
}

void
initFrontEnd(std::string Str)
{
    makeBinOpTable();
    makeTypePrecTable();
    makeWidthTable();
    makeEnvRootTop();
    makeRtErrorTable();

    std::string tmp_Str = ("" == Str)?"std::cin":Str; 
    std::cout << "-----------------------------------------------\n";
    std::cout << "code generated for " << tmp_Str << "\n";
    std::cout << "-----------------------------------------------\n";
}

void // change to a list type ***TO DO***
collectParts() // will collect parts (functions, classes, main,...)
{

}

void
startParse(void)
{
    getNextToken();
    pFirst_Node = parseBlock();

    if ( (no_lex_Errors) || (no_par_Errors) || (no_Warnings) )
	std::cerr << "\n";
    std::string plural;
    if (no_lex_Errors){
	std::cerr << "found " << no_lex_Errors << " lexical error";
	plural = ( (1 < no_lex_Errors) )?"s\n":"\n";
	std::cerr << plural;
    }
    if (no_par_Errors){
	std::cerr << "found "<< no_par_Errors << " syntactic/semantic error";
	plural = ( (1 < no_par_Errors) )?"s\n":"\n";
	std::cerr << plural;
    }
    if (no_Warnings){
	std::cerr << "found "<< no_Warnings << " warning";
	plural = ( (1 < no_Warnings) )?"s\n":"\n";
	std::cerr << plural;
    }
    std::cout << "\n";
}

void
astToIR(void)
{
    if ( (0 != pFirst_Node) ){

	printSTInfo();
	MakeIR_Visitor* IR_Root = new MakeIR_Visitor();
	pFirst_Node->accept(IR_Root);

	if (emitRtError_Section)
	    printDataSection();

	if ( !(option_OptLevel) )	
	    printIR_List(iR_List);

	if ( (1 == option_OptLevel) ){
	    iR_List_2 = removeNOPs(iR_List);
	    printIR_List(iR_List_2);
	}
//	delete IR_Root; // ** TO DO: clarify why can't delete
    }
    else{
	std::cerr << "---no valid statements found---\n";
	return;
    }

    if (emitRtError_Section){
	makeRtErrorTargetTable(iR_RtError_Targets);
	printIR_List(iR_RtError_Targets);
    }
}
