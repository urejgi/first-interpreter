#ifndef BUILTINS_H_
#define BUILTINS_H_

#pragma once

#include <string>

#include "expr.hpp"

static bool equal(const Expr& obj1, const Expr& obj2);
static bool equal_atoms(const Atom* atom1, const Atom* atom2);
static bool equal_cons(const Cons* cons1, const Cons* cons2)
bool nil_p(const Expr& obj);
bool symbol_p(const Expr& obj);
bool string_p(const Expr& obj);
bool integer_p(const Expr& obj);
bool real_p(const Expr& obj);
bool cons_p(const Expr& obj);
bool list_p(const Expr& obj);
bool list_of_symbols_p(const Expr& obj);
bool lambda_p(const Expr& obj);

bool is_special(const std::string& name);

long int length_of_list(const Expr& obj);

// Expr assoc(const Expr& key, const Expr& alist);

Expr list(const std::string& format, ...);

Expr bool_as_expr(bool condition);

#endif  // BUILTINS_H_