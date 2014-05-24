/********************************************************************
* tables.cpp - Tables for Decaf
*
*      binOP_Table: table for Operator Precedence parsing of binary
*                   operator infix expressions
*      typePrec_Table: (basic) type precedence in coercions
*      typeWidth_Table: width of types (bytes) on this machine
*      Env*: linked list of compile-time frames
*      root_Env: root of a (one-sided) linked list of compile-time
*                symbol tables (linking back, to enclosing scope)
*      top_Env: pointer to current Activation Block
*      ST: run-time symbol table (for use of backend)
*
********************************************************************/

#include <sstream>
#include <map>
#include <string>

#include "lexer.h"
#include "tables.h"

// compile-time globals
std::map<IdExpr_AST*, int> prefix_Table;  // accumulate here; copy to Expr_AST
std::map<IdExpr_AST*, int> postfix_Table; // as private variables later
std::map<tokenType, int> binOP_Table;
std::map<std::string, int> typePrec_Table;
std::map<std::string, int> typeWidth_Table;
Env* root_Env;
Env* top_Env; // currently active environment table

// run-time globals
std::map<std::string, Symbol_Table> ST;

// static int used in Symbol Table maintenance in file tables.h
int Env::count_ = -1; // associate 0 with never-used root_Env pointer

// manage pre- and post-increment global (tmp) table (a++, ++a, a--, --a)
// assumes reasonable value V is handed on by caller (+1, -1)
void
idModInsert(std::map<IdExpr_AST*, int>& Type, IdExpr_AST* Name, int V)
{
    if ( (Type.end() != Type.find(Name)) )
	Type[Name] = V;
    else
	Type[Name] += V;
}

// the following tokens have a precedence priority, but are not tracked
// using this table:
// tok_log_not (!), tok_minus (- unary), tok_sqopen ([), tok_dot (.)
void
makeBinOpTable(void)
{
    binOP_Table[tok_eq] = 100;
    binOP_Table[tok_log_or] = 200;
    binOP_Table[tok_log_and] = 300;
    binOP_Table[tok_log_eq] = 400;
    binOP_Table[tok_log_ne] = 400;
    binOP_Table[tok_lt] = 500;
    binOP_Table[tok_le] = 500;
    binOP_Table[tok_gt] = 500;
    binOP_Table[tok_ge] = 500;
    binOP_Table[tok_plus] = 600;
    binOP_Table[tok_minus] = 600;
    binOP_Table[tok_mult] = 700;
    binOP_Table[tok_div] = 700;
    binOP_Table[tok_mod] = 700;
}

int
opPriority(token t)
{
    if ( (binOP_Table.end() != binOP_Table.find(t.Tok())) )
	return binOP_Table[t.Tok()];
    else // this will stop OpPrecedence parsing once we hit a
	return -1;  // non-op while evaluating InfixEpxr
}

// Prepare type_Table with basic types and their coercion priority
// (void is not a legal type for var declaration).
// As we'll also use the table for a 'valid type' check, we added string
// (which cannot be converted to any other type)
void
makeTypePrecTable(void)
{
    token t = token(tok_int);
    typePrec_Table[t.Lex()] = 10;
    t = token(tok_double);
    typePrec_Table[t.Lex()] = 20;
}

int
typePriority(std::string const& Type)
{
    if ( (typePrec_Table.end() != typePrec_Table.find(Type)) )
	return typePrec_Table[Type];
    else
	return -1;
}

// type width in bytes
void
makeWidthTable(void)
{
    typeWidth_Table["int"] = TYPE_WIDTH_INT;
    typeWidth_Table["double"] = TYPE_WIDTH_FLT;
}

// add class width in bytes, as needed (no shadowing assumed)
void
addToWidthTable(std::string Name, int Width)
{
    if ( (typeWidth_Table.end() == typeWidth_Table.find(Name)) )
	typeWidth_Table[Name] = Width;
    else
	errExit(0, "shadowing class definitions not allowed");
}

int
typeWidth(const std::string& Type)
{
    if ( (typeWidth_Table.end() != typeWidth_Table.find(Type)) )
	return typeWidth_Table[Type];
    else
	return -1;
}

Env* 
makeEnvRootTop(void)
{
    return ( (top_Env = root_Env = new Env(0)) );
}

// Builder to maintain parallel compile-time and run-time info about 
// variables in a scope (new link in ct ll rooted at root_Env; new entry
// into rt map ST)
Env*
addEnv(Env* Prior)
{
    Env* pNew_Env = new Env(Prior);
    std::string new_Name = pNew_Env->getTableName();
    Symbol_Table newST = Symbol_Table(new_Name);
    ST[new_Name] = newST;

    return ( (top_Env = pNew_Env) );
}

// pEnv will be used during compile-time, so go via this ll
// Use: only for objects for which space can be allocated at compile-time
//      ( basic types; arrays with integer indices )
int
addDeclToEnv(Env* pEnv, Decl_AST* new_Id, std::string MemType)
{
    if ( (0 == pEnv) || (root_Env == pEnv) ) return -1;
    std::string Name = new_Id->Name();
    // add to Env* entry of Env ll rooted at root_Env
    if ( (0 == pEnv->findName(Name) ) ) // already in tables
	return -1;
    pEnv->insertName(Name, new_Id);

    // add into rt table ST, in the sub-table determined through 
    // the matching Env* pointer into the corresponding ct ll above
    std::string table_Name = pEnv->getTableName();
    std::map<std::string, Symbol_Table>::iterator iter;
    std::string Type(new_Id->Type().Lex());
    int Width(new_Id->Width());

    if ( (ST.end() == (iter = ST.find(table_Name))) )
	return -2;
    (iter->second).insertName(Name, Type, MemType, Width);

    return 0;
}

Decl_AST*
findVarByName(Env* p, std::string Name)
{
    while ( (root_Env != p) ){
	if ( (0 == p->findName(Name)) )
	    return p->readName(Name);
	p = p->getPrior();
    }

    return 0;
}

Decl_AST*
findVarById(Env* p, IdExpr_AST* Id)
{
    std::string name_Str(Id->Addr());
    return findVarByName(p, name_Str);
}

Env*
findVarFrame(Env* p, std::string Name)
{
    while ( (root_Env != p) ){
	if ( (0 == p->findName(Name)) )
	    return p;
	p = p->getPrior();
    }

    return 0;
}

void
printEnvAncestorInfo(Env* p)
{
    if ( (0 == p) ) return;
    while ( (root_Env != p) ){
	std::cout << "Info for table " << p->getTableName() << "\n";
	std::cout << "-----------------------------------\n";
	std::map<std::string, Decl_AST*>::const_iterator iter; 
	std::map<std::string, Decl_AST*> tmp_Type = p->getType();
	for (iter = tmp_Type.begin(); iter != tmp_Type.end(); iter++)
	    std::cout << iter->first << "\t= " << (iter->second)->Type().Lex() 
		      << "\n";

	std::cout << "\n";
	p = p->getPrior();
    }
}

void
printSTInfo()
{
    if ( ST.empty() ) return;
    std::map<std::string, Symbol_Table>::const_iterator iter_Outer;
    for (iter_Outer = ST.begin(); iter_Outer != ST.end(); iter_Outer++){
	std::cout << "Info for table " << iter_Outer->first << "\n";
	std::cout << "---------------------------------------------------\n";

	std::ostringstream tmp_Stream;
	tmp_Stream.width(20);
	tmp_Stream << "Memory allocation - ";
	tmp_Stream.width(7);
	tmp_Stream << "stack: ";
	tmp_Stream << iter_Outer->second.getOffsetStack() << "\n";
	tmp_Stream.width(20);
	tmp_Stream << "";
	tmp_Stream.width(7);
	tmp_Stream << "heap: ";
	tmp_Stream << iter_Outer->second.getOffsetHeap() << "\n\n";

	std::cout << tmp_Stream.str();

	Symbol_Table tmpST(iter_Outer->second);
	std::map<std::string, Mem_Info> info(tmpST.getInfo());
	std::map<std::string, Mem_Info>::const_iterator iter_Inner;
	for (iter_Inner = info.begin(); iter_Inner != info.end(); iter_Inner++){
	    std::string name(iter_Inner->first);
	    std::cout << name;

	    std::cout << "\tType: " << tmpST.getType(name) << "\n";
	    std::cout << "\tMemType: " << tmpST.getMemType(name) << "\n";
	    std::cout << "\tOffset: " << tmpST.getOffset(name) << "\n";
	    std::cout << "\tWidth: " << tmpST.getWidth(name) << "\n";
	std::cout << "\n";
	}
    }
}
