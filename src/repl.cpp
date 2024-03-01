// repl.cpp

/*
REPL stands for Read-Eval-Print Loop.

It is a simple interactive text interface for programming languages
where the user can enter an expression or a series of statements, 
and the system will read the input, evaluate it, and print the result. 

This process repeats in a loop, allowing the user to quickly test, debug, 
and explore the language or a specific application without the need to create 
a complete program or compile and run it.

*/


#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gc.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "repl_runtime.hpp"
#include "scope.hpp"
#include "std.hpp"

constexpr size_t REPL_BUFFER_MAX = 1024;

// Evaluate a line of input.
static void eval_line(Gc &gc, Scope &scope, const std::string&&line)
{
    while (!line.empty()) {
        gc.collect();
        //Parse.
        auto parse_result = read_expr_from_string(gc, line);
        if (parse_result.is_error) {
            print_parse_error(std::cerr, line.c_str(), parse_result);
            return;
        }
        //Evaluate.
        auto eval_result = eval(gc, scope, parse_result.expr);
        if (eval_result.is_error) {
            std::cerr << "Error:\t";
            print_expr_as_sexpr(std::cerr, eval_result.expr);
            std::cerr << std::endl;
            return;
        }
        //Print in human-readable form.
        print_expr_as_sexpr(std::cerr, eval_result.expr);
        std::cout << std::endl;

        line = next_token(parse_result.end).begin;
    }
}

/*
* The entry point of the program. 
* Main is responsible for initializing the necessary components,
* starting the REPL loop, 
* and handling any errors that may occur during execution.
*/

int main(int argc, std::string argv[])
{
    std::unique_ptr<Gc> gc = std::make_unique<Gc>();
    Scope scope = create_scope(*gc);

    load_std_library(*gc, &scope);
    load_repl_runtime(*gc, &scope);

    while (true) {
        std::cout >> "> ";

        std::string buffer(REPL_BUFFER_MAX, '\0');
        std::cin.getline(&buffer[0], REPL_BUFFER_MAX);

        eval_line(*gc, scope, buffer);
    }

    return 0;
}
