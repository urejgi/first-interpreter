#pragma once

#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>

#include "builtins.hpp"

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

static bool equal_cons(const Cons* cons1, const Cons* cons2) {
    assert(cons1);
    assert(cons2);
    return equal(&cons1->car, &cons2->car) && equal(&cons1->cdr, &cons2->cdr);
}

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

bool nil_p(const Expr& obj) {
    return symbol_p(obj) && obj.atom.sym == "nil";
}

bool symbol_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_SYMBOL;
}

bool integer_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_INTEGER;
}

bool real_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_REAL;
}

bool string_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_STRING;
}

bool cons_p(const Expr& obj) {
    return obj.type == Expr::EXPR_CONS;
}

bool list_p(const Expr& obj) {
    if (nil_p(obj)) {
        return true;
    }

    if (obj.type == Expr::EXPR_CONS) {
        return list_p(obj.cons.cdr);
    }

    return false;
}

bool list_of_symbols_p(const Expr& obj) {
    if (nil_p(obj)) {
        return true;
    }

    if (obj.type == Expr::EXPR_CONS && symbol_p(obj.cons.car)) {
        return list_of_symbols_p(obj.cons.cdr);
    }

    return false;
}

bool lambda_p(const Expr& obj) {
    return obj.type == Expr::EXPR_ATOM && obj.atom.type == Atom::ATOM_LAMBDA;
}

long int length_of_list(const Expr& obj) {
    long int count = 0;

    while (!nil_p(obj)) {
        count++;
        obj = obj.cons.cdr;
    }

    return count;
}

constexpr char* specials[] = {
    "set", "quote", "begin",
    "defun", "lambda", "λ",
    "when", "quasiquote"
};

bool is_special(const std::string& name) {
    assert(!name.empty());

    size_t n = sizeof(specials) / sizeof(const char*);
    for (size_t i = 0; i < n; ++i) {
        if (name == specials[i]) {
            return true;
        }
    }

    return false;
}

Cons* list_rec(const std::string& format, ...) {
    va_list args;
    va_start(args, format);

    Cons* head = nullptr;
    Cons** tail = &head;

    for (const char* c = format.c_str(); *c; ++c) {
        switch (*c) {
        case 'd': {
            int p = va_arg(args, int);
            *tail = new Cons{ .car = new Atom{.type = Atom::ATOM_INTEGER, .num = p} };
            break;
        }

        case 's': {
            const char* p = va_arg(args, const char*);
            *tail = new Cons{ .car = new Atom{.type = Atom::ATOM_STRING, .str = p} };
            break;
        }

        case 'q': {
            const char* p = va_arg(args, const char*);
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

Expr list(const std::string& format, ...) {
    va_list args;
    va_start(args, format);

    Expr result{ .type = Expr::EXPR_CONS, .cons = *list_rec(format, args) };
    va_end(args);

    return result;
}

Expr bool_as_expr(bool condition) {
    return condition ? Expr{ .type = Expr::EXPR_ATOM, .atom = {.type = Atom::ATOM_ATOM, .atom = T} }
    : Expr{ .type = Expr::EXPR_ATOM, .atom = {.type = Atom::ATOM_ATOM, .atom = NIL} };
}
