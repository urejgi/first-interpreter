#ifndef SCOPE_H_
#define SCOPE_H_

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

Expr get_scope_value(const Scope* scope, const Expr& name);
void set_scope_value(Gc* gc, Scope* scope, const Expr& name, const Expr& value);
void push_scope_frame(Gc* gc, Scope* scope, const Expr& vars, const Expr& args);
void pop_scope_frame(Gc* gc, Scope* scope);

#endif  // SCOPE_H_
