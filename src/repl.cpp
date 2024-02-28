// repl.cpp

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

static void eval_line(Gc &gc, Scope &scope, const std::string &line)
{
    while (!line.empty()) {
        gc.collect();

        auto parse_result = read_expr_from_string(gc, line);
        if (parse_result.is_error) {
            print_parse_error(std::cerr, line.c_str(), parse_result);
            return;
        }

        auto eval_result = eval(gc, scope, parse_result.expr);
        if (eval_result.is_error) {
            std::cerr << "Error:\t";
            print_expr_as_sexpr(std::cerr, eval_result.expr);
            std::cerr << std::endl;
            return;
        }

        print_expr_as_sexpr(std::cerr, eval_result.expr);
        std::cout << std::endl;

        line = next_token(parse_result.end).begin;
    }
}

int main(int argc, char *argv[])
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
