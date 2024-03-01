#ifndef SCOPE_H_
#define SCOPE_H_

#pragma once

#include "expr.hpp"
#include "builtins.hpp"

struct Scope
{
    Expr expr;
};

// Scope is a stack of alists
// (((y . 20))
//  ((x . 10)
//   (name . "Alexey")))

Scope create_scope(Gc* gc);

Expr get_scope_value(const Scope* scope, Expr name);
void set_scope_value(Gc* gc, Scope* scope, Expr name, Expr value);
void push_scope_frame(Gc* gc, Scope* scope, Expr vars, Expr args);
void pop_scope_frame(Gc* gc, Scope* scope);

#endif  // SCOPE_H_
