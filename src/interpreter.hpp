#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#pragma once

#include <stdexcept>
#include <string>
#include <system_error>
#include <cstdarg>

#include "builtins.hpp"
#include "expr.hpp"
#include "scope.hpp"
#include "gc.hpp"

EvalResult eval_failure(Expr error);

EvalResult wrong_argument_type(Gc* gc, const char* type, Expr obj);

EvalResult wrong_integer_of_arguments(Gc* gc, long int count);

EvalResult not_implemented(Gc* gc);

EvalResult read_error(Gc* gc, const char* error_message, long int character);

EvalResult car(Gc* gc, Scope* scope, Expr args);

EvalResult eval(Gc* gc, Scope* scope, Expr expr);

EvalResult eval_block(Gc* gc, Scope* scope, Expr block);

EvalResult match_list(Gc* gc, const char* format, Expr xs, ...);

#endif  // INTERPRETER_H_

