// builtins.cpp

/*
The code in this file provides essential functionality for a Lisp interpreter,
enabling it to distinguish between diffeernt type of expressions, 
check for their equality, and identify special forms.
*/

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>
#include <stdarg.h>

#include "builtins.hpp"


// Expression Equality.


/*
*  Compares two atoms to see if they are equal.
    Atoms can be symbols, integers, real numbers, strings, lambda functions, or native functions.

    Equality conditions vary based on the atom type, e.g., an epsilon comparison is used for real numbers 
    to handle floating-point precision issues.
*/
static bool equal_atoms(const Atom* atom1, const Atom* atom2) {
    assert(atom1);
    assert(atom2);

    if (atom1->type != atom2->type) {
        return false;
    }

    switch (atom1->type) {
    case Atom::ATOM_SYMBOL:
        return atom1->sym == atom2->sym;

    case Atom::ATOM_INTEGER:
        return atom1->num == atom2->num;

    case Atom::ATOM_REAL:
        return std::abs(atom1->real - atom2->real) < 1e-6;

    case Atom::ATOM_STRING:
        return atom1->str == atom2->str;

    case Atom::ATOM_LAMBDA:
        return atom1 == atom2;

    case Atom::ATOM_NATIVE:
        return atom1->native == atom2->native;
    }

    return false;
}

/*
* Compares two cons cells for equality by recursively checking equality of both their car and cdr parts.
    
    Cons is the fundamental building blocks of lists in Lisp, cons cells have two parts: car and cdr, 
    representing the first element and the rest of the list, respectively.
    
*/
static bool equal_cons(const Cons* cons1, const Cons* cons2) {
    assert(cons1);
    assert(cons2);
    return equal(&cons1->car, &cons2->car) && equal(&cons1->cdr, &cons2->cdr);
}

/*
* Determines if two expressions are equal by comparing their types 
    (atom, cons cell, or void) and then delegating to equal_atoms or equal_cons as appropriate.
*/
bool equal(const Expr& obj1, const Expr& obj2) {
    if (obj1.type != obj2.type) {
        return false;
    }

    switch (obj1.type) {
    case Expr::EXPR_ATOM:
        return equal_atoms(&obj1.atom, &obj2.atom);

    case Expr::EXPR_CONS:
        return equal_cons(&obj1.cons, &obj2.cons);

    case Expr::EXPR_VOID:
        return true;
    }

    return true;
}


// Type Checks.

/*
* - The *_p functions (nil_p, symbol_p, integer_p, real_p, string_p, cons_p, list_p, list_of_symbols_p, lambda_p) 
    are predicates used to check the type of an expression.
    
    They take an object of Expr type and check if this object is their type.
*/


// Check if an expression is nil.
bool nil_p(const Expr& obj) {
    return symbol_p(obj) && obj.atom.sym == "nil";
}

// Check if an expression is symbol.
bool symbol_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_SYMBOL;
}

// Check if an expression is an integer.
bool integer_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_INTEGER;
}

// Check if an expression is a real.
bool real_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_REAL;
}

// Check if an expression is string.
bool string_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_STRING;
}

// Check if an expression is a cons.
bool cons_p(const Expr& obj) {
    return obj.type == Expr::EXPR_CONS;
}

// Check if an expression is a list.
bool list_p(const Expr& obj) {
    if (nil_p(obj)) {
        return true;
    }

    if (obj.type == Expr::EXPR_CONS) {
        return list_p(obj.cons.cdr);
    }

    return false;
}

// Check if an expression is a list of symbols.
bool list_of_symbols_p(const Expr& obj) {
    if (nil_p(obj)) {
        return true;
    }

    if (obj.type == Expr::EXPR_CONS && symbol_p(obj.cons.car)) {
        return list_of_symbols_p(obj.cons.cdr);
    }

    return false;
}

// Check if an expression is a lambda.
bool lambda_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_LAMBDA;
}

// Calculate length of the list.
long int length_of_list(const Expr& obj) {
    long int count = 0;

    while (!nil_p(obj)) {
        count++;
        obj = obj.cons.cdr;
    }

    return count;
}

// Some special forms.
constexpr std::string specials[] = {
    "set", "quote", "begin",
    "defun", "lambda", "λ",
    "when", "quasiquote"
};

// Check if a string is a special form.
bool is_special(const std::string& name) {
    assert(!name.empty());
    return binary_search(specials.begin(), specials.end(), name);
}

/*
* The list_rec function aims to create a linked list (a cons list in Lisp terms) from a given format string and a variable number of arguments. The format string specifies the types of objects that will be inserted into the list:

- 'd' indicates an integer (ATOM_INTEGER).
- 's' indicates a string (ATOM_STRING).
- 'q' indicates a symbol (ATOM_SYMBOL).
- 'e' indicates an expression should be inserted directly.

    The function iterates over the characters in the format string, 
    creating a new atom for each character based on the specified type 
    and inserting it into the cons cell at the tail of the list.
*/

Cons* list_rec(const std::string& format, ...) {
    va_list args;
    va_start(args, format);

    Cons* head = nullptr;
    Cons** tail = &head;

    for (const std::string c = format.c_str(); *c; ++c) {
        switch (*c) {
        case 'd': {
            int p = va_arg(args, int);
            *tail = new Cons{ .car = new Atom{.type = Atom::ATOM_INTEGER, .num = p} };
            break;
        }

        case 's': {
            const std::string p = va_arg(args, const std::string);
            *tail = new Cons{ .car = new Atom{.type = Atom::ATOM_STRING, .str = p} };
            break;
        }

        case 'q': {
            const std::string p = va_arg(args, const std::string);
            *tail = new Cons{ .car = new Atom{.type = Atom::ATOM_SYMBOL, .sym = p} };
            break;
        }

        case 'e': {
            Expr p = va_arg(args, Expr);
            *tail = new Cons{ .car = new Atom{.type = Atom::ATOM_ATOM, .atom = p.atom} };
            break;
        }

        default:
            throw std::runtime_error("Wrong format parameter");
        }

        tail = &(*tail)->cdr;
    }

    *tail = nullptr;
    va_end(args);

    return head;
}

/*
* The list function is a wrapper around list_rec 
    but returns an Expr type, making it suitable for directly interacting with the Lisp interpreter's environment.

    It uses the list_rec function to construct a cons cell from the format string and arguments,
    and then wraps this list in an Expr typed structure.
*/

Expr list(const std::string& format, ...) {
    va_list args;
    va_start(args, format);

    Expr result{ .type = Expr::EXPR_CONS, .cons = *list_rec(format, args) };
    va_end(args);

    return result;
}

/*
* The bool_as_expr function takes a boolean value and returns a Lisp expression that represents a Lisp boolean.
*/
Expr bool_as_expr(bool condition) {
    return condition ? Expr{ .type = Expr::EXPR_ATOM, .atom = {.type = Atom::ATOM_ATOM, .atom = T} }
    : Expr{ .type = Expr::EXPR_ATOM, .atom = {.type = Atom::ATOM_ATOM, .atom = NIL} };
}
    