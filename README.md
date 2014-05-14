Compiler for the Brown Decaf language. Note that the language appears to be modified frequently. This project is based on the specifications of a Stanford 2012 class. Some changes were made (eventually, a full CFG grammar will be added):

(1) visibility of declarations and definitions is C/C++-like - from the point 
    of declaration; use before declaration is error-checked,

(2) use of uninitialized variables: emits warning (but use is not disallowed) - 
    if used unitialized several times, warning emitted only at first use;
    note that if a variable is only initialized in a conditionally taken
    branch ( int a, b; if expr (a = 1); b = a; ), we do not emit a warning;
    however, see "arrays" for the case of arrays,

(3) type coercion/conversion: for basic types, as in C/C++,  

(4) strings: debating copy semantics; syntax by and large as in C,   

(5) types: bool currently disallowed; octal integers legal; double must be
           decimal,

(6) arrays: integer expressions bounds, bound-checked (as per CFG); but
            placeable on both stack and heap. In case of integer expression
            bounds (as opposed to integers proper), combined with stack
            allocation, this leads to run-time (visitor level) stack
            growth (entering scope), and shrinking (leaving scope).

    Declaration-time initialization: not implemented.

    Note:  if int a[4], 'a' is a pointer to the location in memory where
           this array will be (consecutively) stored. As pointers are
           currently not implemented, any use of 'a' in an expression or
           as an LValue is illegal - the only legal access is 'a[i]') (the
           location is resolved as in C; but pointers proper are not
           implemented).

    Init. checks: none. This is extremely hard to implement generally.
                  (asking on SO about best practice yielded as feedback
                  calling it "A road of pain"). Hence, a[(expr)] is not
                  initialization checked (after emitting bound-check code,
                  retrieve the value at the implied memory location),

(7) assignment expressions: can be chained,
 
(8) logical expressions: evaluation similar to C (this might mean similarly
    undefined behavior; tbd),

(9) short-circuiting of ||/&&: in the usual C/C++/Java way,   

(10) for: all 3 expressions in expression list are optional (middle one too)

Eventually, the compiler will have multiple parse rounds, on either the AST, or the generated SSA-style IR. Currently, the implemented optimization rounds are:
-O 0: remove NOPs from IR.

After a sufficiently large part of the language can be translated to an IR, we will add an x86 32b assembly interpreter as a backend (maybe only for a subset of the language).