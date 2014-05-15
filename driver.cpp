/********************************************************************
* driver.cpp - driver for Decaf
*
********************************************************************/

#include <sstream> // include max. # of #include files to monitor dependencies
#include <cstdio>
#include "lexer.h"
#include "ast.h"
#include "error.h"
#include "tables.h"
#include "parser.h"
#include "ir.h" 
#include "visitor.h"

extern Node_AST* pFirst_Node; // double declaration (from ast.h) - for clarity

extern int no_lex_Errors;
extern int no_par_Errors;
extern int emitRtError_Section;

std::istream* input;
int option_Debug = 0;
int option_optLevel = 0; // 0 - remove NOPs

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
	std::vector<RtError_Type*> t;	
	for ( iter = t.begin(); iter != t.end(); iter++ )
	    if ( (0 != *iter) ) delete *iter;
    }

    if ( !(Ds_Table.empty()) ){
	std::vector<Ds_Object*>::iterator iter;
	std::vector<Ds_Object*> t;
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

void
cleanUp(void)
{
    deallocate(pFirst_Node);
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

// ** TO DO: name no longer reflectes its use; consider breaking into pieces
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

    if ( (0 != pFirst_Node) ){
	printSTInfo();
	pFirst_Node->accept(new MakeIR_Visitor);

	if (emitRtError_Section)
	    printDataSection();

	if ( !(option_optLevel) )	
	    printIR_List(iR_List);

	if ( (1 == option_optLevel) ){
	    iR_List_2 = removeNOPs(iR_List);
	    printIR_List(iR_List_2);
	}
   }
    else
	std::cerr << "---no valid statements found---\n";

    if (emitRtError_Section){
	makeRtErrorTargetTable(iR_RtError_Targets);
	printIR_List(iR_RtError_Targets);
    }
}

