Compiler for the Brown Decaf language. Note that the language appears to be modified frequently. This project is based on the specifications of a Stanford 2012 class. Some changes were made: 

(1) visibility of declarations and definitions is C/C++-like - from the point of declaration,  

(2) basic types type coercion/conversion was added,  

(3) we short-circuit in the usual way,   

(4) debating copy semantics of strings,   

(5) type bool currently disallowed,  
 
(6) evaluation of logical expressions similar to C (1 - true; 0 - false),  

(7) there are 2 prefix operators (-, !); we disallow: (a) twice the same 
    in a row, and (b) 3 in a row (could easily be added).

Eventually, the compiler will have multiple parse rounds, probably refining first the AST, and then the generated SSA-style IR, using a Visitor Pattern to manage at least AST parsing but probably also the DAG implied by the IR.

When an IR can be produced for the entire CFG, and prior to AST/IR optimization rounds, we will add an x86 32b assembly interpreter as a backend (maybe only for a subset of the language).