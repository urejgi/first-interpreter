This project is an interpreter for a Lisp-like language. 

The structure of the project, the hierarchy of files is represented below.

interpreter/
├── parsing_and_tokenization/
│   ├── parser.cpp        # Parses source code into an abstract syntax tree.
│   ├── tokenizer.cpp     # Breaks down the source into tokens.
│   └── expr.cpp          # Defines the structure of expressions.
├── evaluation/
│   └── interpreter.cpp   # Interprets the abstract syntax tree.
├── standard_library_and_built_in_infrastructure/
│   ├── std.cpp           # Standard library functions and utilities.
│   └── builtins.cpp      # Implementation of built-in functions and constructs.
├── memory_management/
│   └── gc.cpp            # Garbage collection and memory management.
├── helpers/
│   ├── str.cpp           # String manipulation and utility functions.
│   └── scope.cpp         # Manages scoping rules and environments.
└── user_interaction/
    ├── repl.cpp          # Read-eval-print loop for interactive use.
    └── repl-runtime.cpp  # Runtime environment for the REPL.



The files are organized into three main categories: built-ins, standard library, and the interpreter itself. 

1) Built-ins: 
    These are the basic functions and operations that the language supports. 
    They are defined in the files builtins.cpp and builtins.hpp. 

2) Standard Library: 
    These are additional functions and operations that are not part of the core language 
    but are commonly used.
    They are defined in the files std.cpp and std.hpp.

3) Interpreter: These are the files that implement the interpreter itself.
    They are responsible for parsing the input, evaluating the expressions, and managing the memory. 



The main file is repl.cpp, which is the entry point of the program.
It initializes the garbage collector, scope, and standard library.
It then enters a loop where it reads a line of input, 
parses it into an expression, evaluates it, and prints the result.

The parser.cpp and parser.hpp files are responsible for parsing the input into an expression. 

The interpreter.cpp and interpreter.hpp files are responsible for evaluating expressions. 

The gc.cpp and gc.hpp files are responsible for managing memory. 
They define a garbage collector that can be used to allocate and deallocate memory.

The scope.cpp and scope.hpp files are responsible for managing the scope of variables. 
They define a scope that can be used to store and look up variables.

The repl_runtime.cpp and repl_runtime.hpp files are responsible for loading 
the standard library and built-in functions at runtime. 
They define functions that can be called to load the standard library and built-in functions into a scope.

The str.cpp and str.hpp files are responsible for string manipulation. 
They define functions for tokenizing strings, finding the next non-symbol character,
and printing expressions as S-expressions.

The tokenizer.cpp and tokenizer.hpp files are responsible for tokenizing strings. 

Each of the code files has comments within, so you can easily find out for what each part is responsible for.
