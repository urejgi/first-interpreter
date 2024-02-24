// expr.cpp

#include <assert.h>
#include <stdio.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <expr.hpp>

Expr atom_as_expr(Atom *atom)
{
    Expr expr = {
        .type = EXPR_ATOM,
        .atom = atom
    };

    return expr;
}

Expr cons_as_expr(Cons *cons)
{
    Expr expr = {
        .type = EXPR_CONS,
        .cons = cons
    };

    return expr;
}

Expr void_expr(void)
{
    Expr expr = {
        .type = EXPR_VOID
    };

    return expr;
}

void print_atom_as_sexpr(FILE *stream, Atom *atom)
{
    assert(atom);

    switch (atom->type) {
    case ATOM_SYMBOL: {
        fprintf(stream, "%s", atom->sym);
    } break;

    case ATOM_INTEGER: {
        fprintf(stream, "%ld", atom->num);
    } break;

    case ATOM_REAL: {
        fprintf(stream, "%f", atom->real);
    } break;

    case ATOM_STRING: {
        fprintf(stream, "\"%s\"", atom->str);
    } break;

    case ATOM_LAMBDA: {
        fprintf(stream, "<lambda %s>", atom->lambda.args_list.atom->sym);
    } break;

    case ATOM_NATIVE: {
        fprintf(stream, "<native>");
    } break;
    }
}

void print_cons_as_sexpr(FILE *stream, Cons *head)
{
    assert(head);

    Cons *cons = head;

    fprintf(stream, "(");
    print_expr_as_sexpr(stream, cons->car);

    while (cons->cdr.type == EXPR_CONS) {
        cons = cons->cdr.cons;
        fprintf(stream, " ");
        print_expr_as_sexpr(stream, cons->car);
    }

    if (cons->cdr.atom->type != ATOM_SYMBOL ||
        strcmp("nil", cons->cdr.atom->sym) != 0) {
        fprintf(stream, " . ");
        print_expr_as_sexpr(stream, cons->cdr);
    }

    fprintf(stream, ")");
}

void print_expr_as_sexpr(FILE *stream, Expr expr)
{
    switch (expr.type) {
    case EXPR_ATOM:
        print_atom_as_sexpr(stream, expr.atom);
        break;

    case EXPR_CONS:
        print_cons_as_sexpr(stream, expr.cons);
        break;

    case EXPR_VOID:
        break;
    }
}

void destroy_expr(Expr expr)
{
    switch (expr.type) {
    case EXPR_ATOM:
        destroy_atom(expr.atom);
        break;

    case EXPR_CONS:
        destroy_cons(expr.cons);
        break;

    case EXPR_VOID:
        break;
    }
}

Cons *create_cons(Gc *gc, Expr car, Expr cdr)
{
    Cons *cons = new Cons;
    cons->car = car;
    cons->cdr = cdr;

    gc_add_expr(gc, cons_as_expr(cons));

    return cons;
}

void destroy_cons(Cons *cons)
{
    delete cons;
}

Atom *create_real_atom(Gc *gc, float real)
{
    Atom *atom = new Atom;
    atom->type = ATOM_REAL;
    atom->real = real;

    if (gc_add_expr(gc, atom_as_expr(atom)) < 0) {
        delete atom;
        return NULL;
    }

    return atom;
}

Atom *create_integer_atom(Gc *gc, long int num)
{
    Atom *atom = new Atom;
    atom->type = ATOM_INTEGER;
    atom->num = num;

    if (gc_add_expr(gc, atom_as_expr(atom)) < 0) {
        delete atom;
        return NULL;
    }

    return atom;
}

Atom *create_string_atom(Gc *gc, const char *str, const char *str_end)
{
    Atom *atom = new Atom;

    if (atom == NULL) {
        goto error;
    }

    atom->type = ATOM_STRING;
    atom->str = string_duplicate(str, str_end);

    if (atom->str == NULL) {
        goto error;
    }

    if (gc_add_expr(gc, atom_as_expr(atom)) < 0) {
        goto error;
    }

    return atom;

error:
    if (atom != NULL) {
        if (atom->str != NULL) {
            delete[] atom->str;
        }
        delete atom;
    }

    return NULL;
}

Atom *create_symbol_atom(Gc *gc, const char *sym, const char *sym_end)
{
    Atom *atom = new Atom;

    if (atom == NULL) {
        goto error;
    }

    atom->type = ATOM_SYMBOL;
    atom->sym = string_duplicate(sym, sym_end);

    if (atom->sym == NULL) {
        goto error;
    }

    if (gc_add_expr(gc, atom_as_expr(atom)) < 0) {
        goto error;
    }

    return atom;

error:
    if (atom != NULL) {
        if (atom->sym != NULL) {
            delete[] atom->sym;
        }
        delete atom;
    }

    return NULL;
}

Atom *create_lambda_atom(Gc *gc, Expr args_list, Expr body, Expr envir)
{
    Atom *atom = new Atom;

    if (atom == NULL) {
        goto error;
    }

    atom->type = ATOM_LAMBDA;
    atom->lambda.args_list = args_list;
    atom->lambda.body = body;
    atom->lambda.envir = envir;

    if (gc_add_expr(gc, atom_as_expr(atom)) < 0) {
        goto error;
    }

    return atom;

error:
    if (atom != NULL) {
        delete atom;
    }

    return NULL;
}

Atom *create_native_atom(Gc *gc, NativeFunction fun, void *param)
{
    Atom *atom = new Atom;

    if (atom == NULL) {
        goto error;
    }

    atom->type = ATOM_NATIVE;
    atom->native.fun = fun;
    atom->native.param = param;

    if (gc_add_expr(gc, atom_as_expr(atom)) < 0) {
        goto error;
    }

    return atom;

error:
    if (atom != NULL) {
        delete atom;
    }

    return NULL;
}

void destroy_atom(Atom *atom)
{
    switch (atom->type) {
    case ATOM_SYMBOL:
    case ATOM_STRING: {
        delete[] atom->str;
    } break;

    case ATOM_LAMBDA:
    case ATOM_NATIVE:
    case ATOM_INTEGER:
    case ATOM_REAL: {
        /* Nothing */
    } break;
    }

    delete atom;
}

int atom_as_sexpr(Atom *atom, char *output, size_t n)
{
    assert(atom);
    assert(output);

    switch (atom->type) {
    case ATOM_SYMBOL: {
        return snprintf(output, n, "%s", atom->sym);
    }

    case ATOM_INTEGER: {
        return snprintf(output, n, "%ld", atom->num);
    }

    case ATOM_REAL: {
        return snprintf(output, n, "%f", atom->real);
    }

    case ATOM_STRING: {
        return snprintf(output, n, "\"%s\"", atom->str);
    }

    case ATOM_LAMBDA:
        return snprintf(output, n, "<lambda>");

    case ATOM_NATIVE:
        return snprintf(output, n, "<native>");
    }

    return 0;
}

int cons_as_sexpr(Cons *head, char *output, size_t n)
{
    assert(head);
    assert(output);

    /* TODO(#378): cons_as_sexpr does not handle encoding errors of snprintf */

    Cons *cons = head;

    int m = (int) n;

    int c = snprintf(output, n, "(");
    if (m - c <= c) {
        return c;
    }

    c += expr_as_sexpr(cons->car, output + c, (size_t) (m - c));
    if (m - c <= 0) {
        return c;
    }

    while (cons->cdr.type == EXPR_CONS) {
        cons = cons->cdr.cons;

        c += snprintf(output + c, (size_t) (m - c), " ");
        if (m - c <= 0) {
            return c;
        }

        c += expr_as_sexpr(cons->car, output + c, (size_t) (m - c));
        if (m - c <= 0) {
            return c;
        }
    }

    if (cons->cdr.atom->type != ATOM_SYMBOL ||
        strcmp("nil", cons->cdr.atom->sym) != 0) {

        c += snprintf(output + c, (size_t) (m - c), " . ");
        if (m - c <= 0) {
            return c;
        }

        c += expr_as_sexpr(cons->cdr, output + c, (size_t) (m - c));
        if (m - c <= 0) {
            return c;
        }
    }

    c += snprintf(output + c, (size_t) (m - c), ")");
    if (m - c <= 0) {
        return c;
    }

    return c;
}

int expr_as_sexpr(Expr expr, char *output, size_t n)
{
    switch(expr.type) {
    case EXPR_ATOM:
        return atom_as_sexpr(expr.atom, output, n);

    case EXPR_CONS:
        return cons_as_sexpr(expr.cons, output, n);

    case EXPR_VOID:
        return 0;
    }

    return 0;
}

const char *expr_type_as_string(ExprType expr_type)
{
    switch (expr_type) {
    case EXPR_ATOM: return "EXPR_ATOM";
    case EXPR_CONS: return "EXPR_CONS";
    case EXPR_VOID: return "EXPR_VOID";
    }

    return "";
}

const char *atom_type_as_string(AtomType atom_type)
{
    switch (atom_type) {
    case ATOM_SYMBOL: return "ATOM_SYMBOL";
    case ATOM_INTEGER: return "ATOM_INTEGER";
    case ATOM_REAL: return "ATOM_REAL";
    case ATOM_STRING: return "ATOM_STRING";
    case ATOM_LAMBDA: return "ATOM_LAMBDA";
    case ATOM_NATIVE: return "ATOM_NATIVE";
    }

    return "";
}

