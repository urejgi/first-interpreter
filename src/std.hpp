
#ifndef STD_H_
#define STD_H_

#pragma once

#include "gc.hpp"
#include "interpreter.hpp"
#include "builtins.hpp"
#include "scope.hpp"
#include "parser.hpp"
#include "expr.hpp"

void load_std_library(Gc* gc, Scope* scope);

#endif  // STD_H_
