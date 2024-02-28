#ifndef EXPR_H_
#define EXPR_H_

#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "gc.cpp"

class Scope;

struct Cons;
struct Atom;

enum ExprType
{
    EXPR_ATOM = 0,
    EXPR_CONS,
    EXPR_VOID
};

struct Expr
{
    ExprType type;
    union
    {
        Cons* cons;
        Atom* atom;
    };
};

const char* expr_type_as_string(ExprType expr_type);

Expr atom_as_expr(Atom* atom);
Expr cons_as_expr(Cons* cons);
Expr void_expr();

void destroy_expr(Expr expr);
void print_expr_as_sexpr(std::ostream& stream, Expr expr);
void print_expr_as_c(std::ostream& stream, Expr expr);
int expr_as_sexpr(Expr expr, char* output, size_t n);

struct EvalResult
{
    bool is_error;
    Expr expr;
};

using NativeFunction = EvalResult(*)(void* param, Gc* gc, Scope* scope, Expr args);

struct Native
{
    NativeFunction fun;
    void* param;
};

struct Lambda
{
    Expr args_list;
    Expr body;
    Expr envir;
};

enum AtomType
{
    ATOM_SYMBOL = 0,
    ATOM_INTEGER,
    ATOM_REAL,
    ATOM_STRING,
    ATOM_LAMBDA,
    ATOM_NATIVE
};

const char* atom_type_as_string(AtomType atom_type);

struct Atom
{
    AtomType type;
    union
    {
        long int num;           // ATOM_INTEGER
        float real;             // ATOM_REAL
        std::string sym;        // ATOM_SYMBOL
        std::string str;        // ATOM_STRING
        Lambda lambda;         // ATOM_LAMBDA
        Native native;         // ATOM_NATIVE
    };
};

Atom* create_integer_atom(long int num);
Atom* create_real_atom(float num);
Atom* create_string_atom(const char* str, const char* str_end = nullptr);
Atom* create_symbol_atom(const char* sym, const char* sym_end = nullptr);
Atom* create_lambda_atom(Expr args_list, Expr body, Expr envir);
Atom* create_native_atom(NativeFunction fun, void* param);
void destroy_atom(Atom* atom);
void print_atom_as_sexpr(std::ostream& stream, Atom* atom);

struct Cons
{
    Expr car;
    Expr cdr;
};

Cons* create_cons(Expr car, Expr cdr);
void destroy_cons(Cons* cons);
void print_cons_as_sexpr(std::ostream& stream, Cons* cons);

#endif  // EXPR_H_
