#ifndef LEXER_H_
#define LEXER_H_

//#include <string>

/*
void int double bool string class interface null this extends implements
for while if else return break new NewArray Print ReadInteger ReadLine
*/
enum token{
	// misc
	tok_eof = -1, tok_return = -2,
	// types
	tok_void = -10, tok_int = -11, tok_double = -12, tok_bool = -13,
	tok_string = -14, tok_null = -15,
	// looping
	tok_for = -20, tok_while = -21, tok_if = -22, tok_else = -23, 
	tok_break = -24,
	// i/o
	tok_Print = -30, tok_ReadInteger = -31, tok_ReadLine = -32,
	// classes
	tok_class = -40, tok_interface = -41, tok_this = -42, tok_extends = -43,
	tok_implements = -44, 
	// allocation/declaration
	tok_new = -50, tok_NewArray = -51,
	// AST types
	tok_ID = -100, tok_Int = -101, tok_flt = -102, tok_expr = -103,
};

// it is more logical to make them extern here: they are defined in 
// lexer.cpp translation unit
extern std::string identifier_Str;  // for tok_identifier
extern long val_Int;       // for tok_number
extern double val_Flt; 

int getTok(void);

#endif
