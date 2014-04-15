Compiler for the Brown Decaf language. Note that the language appears to be modified frequently. This here is based on the specifications of a Stanford 2012 class. Some changes were made: (1) visibility of declarations and definitions is C/C++-like - from the point of declaration, (2) basic types type coercion/conversion will be added, (3) we will short-circuit in the usual way, (4) debating copy semantics of strings; (5) type bool currently disallowed; (6) evaluation of logical expressions similar to C/C++. Execution goals include multiple parse rounds, probably refining first the AST, and then the generated original SSA-style IR; using a Visitor Pattern class to manage at least AST parsing but probably also the DAG implied by the IR; generating x86 32b assembly in backend once front end complete. Some of this will be done after HS - impossible (I believe) in time remaining. 