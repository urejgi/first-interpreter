#ifndef EXPR_H_
#define EXPR_H_

#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "gc.hpp"

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

const std::string expr_type_as_string(ExprType expr_type);


Expr atom_as_expr(const Atom& atom);
Expr cons_as_expr(const Cons& cons);
Expr void_expr(void);

void destroy_expr(Expr expr);
void print_atom_as_sexpr(FILE* stream, const Atom& atom);
void print_cons_as_sexpr(FILE* stream, Cons* head);
void print_expr_as_sexpr(FILE* stream, Expr expr);

int expr_as_sexpr(Expr expr, std::string output, size_t n);


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

const std::string atom_type_as_string(AtomType atom_type);

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


Atom* create_real_atom(Gc* gc, float real);
Atom* create_integer_atom(Gc* gc, long int num);
Atom* create_string_atom(Gc* gc, const std::string& str, const std::string& str_end);
Atom* create_symbol_atom(Gc* gc, const std::string& sym, const std::string& sym_end);
Atom* create_lambda_atom(Gc* gc,Expr args_list, Expr body, Expr envir);
Atom* create_native_atom(Gc* gc, NativeFunction fun, void* param);

void destroy_atom(Atom* atom);


struct Cons
{
    Expr car;
    Expr cdr;
};

Cons* create_cons(Gc* gc, Expr car, Expr cdr);

void destroy_cons(Cons* cons);
void print_cons_as_sexpr(FILE* stream, Cons* head);

#endif  // EXPR_H_


