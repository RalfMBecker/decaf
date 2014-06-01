CCC (the Cuddly Chameleon Compiler) is a compiler for the Brown Decaf language. Note that the language seems to be modified frequently, and that CCC adds a fair amount of additional features. The project is based on the specifications of a Stanford 2012 class. Eventually, a full CFG grammar will be added.

(0) options provided:

    -d: print debugging output

    -p: only pre-process, and save in <basename>.pre

    -i: only generate IR bytecode, and save in <basename>.ir

    -O: optimize; currently -
        0: remove NOPs from IR,

(1) pre-processing: implemented some steps of C++03 preprocessing directives -

     - remove comments

     - concatenate adjacent strings (2.13.4 (4))

     - add a terminating '\0' to strings (after concatenation, if applicable)
       (2.13.4 (5))

    Lexer operates on pre-processed tmp-file,

(2) we use (greedy) LR parsing, and how tokens are recognized reflects this:
    a+++++b -> a ++ ++ + b -> syntax error
    --b+--a -> -- b + -- a -> --b + --a

(3) use of uninitialized variables: emits warning (but use is not disallowed) - 
    if used unitialized several times, warning emitted only at first use;
    note that if a variable is only initialized in a conditionally taken
    branch ( int a, b; if expr (a = 1); b = a; ), we do not emit a warning;
    however, see "arrays" for the case of arrays,

(4) type coercion/conversion: for basic types, as in C/C++, except special
    case: if LHS is an ArrayIdExpr_AST object (eg, a[i]), we coerce to its
    type when type of LHS differs from type of RHS (as we don't allow for
    heterogenous arrays, either that, or make it an error),

(5) strings: debating copy semantics; syntax by and large as in C,   

(6) types: bool currently disallowed; octal integers legal; double must be
           decimal,

(7) arrays: integer expressions bounds, bound-checked (as per CFG); but
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

(8) assignment expressions: can be chained,
 
(9) logical expressions: evaluation similar to C (this might mean similarly
    undefined behavior; tbd),

(10) short-circuiting of ||/&&: in the usual C/C++/Java way,   

(11) for: all 3 expressions in expression list are optional (middle one too),

(12) visibility of declarations and definitions is C/C++-like - from the point 
    of declaration; use before declaration is error-checked,

(13) added pre- and post-increment operators (++a, a++):

     Pre-fix:  execute as soon as met; use updated value
     Post-fix: save current value in a tmp t1
               update value
               use tmp t1 in the next enclosing expression
     Note:     - We parse L-R. For the post-increment application, 'next' 
               means what will next be processed in parsing.
               - We don't really have undefined behavior.
     Example:  int a = 1;
               a *= a++ + (++a) * a++ + a;
                  = (1 + 3 * 3 + 4) * 4 = 56
     gcc:      probably different; but fails completely in case of a[i]++, etc.
     clang:    seems to do exactly the above (scalar and array case)

(14) added modifying assignments (+=, -=, *=, /=) (c. (13)), 

Eventually, the compiler will have multiple parse rounds, on either the AST, or the generated SSA-style IR. Currently, the implemented optimization rounds are:
-O 0: remove NOPs from IR.

After a sufficiently large part of the language can be translated to an IR, we will add an x86 32b assembly interpreter as a backend (maybe only for a subset of the language).