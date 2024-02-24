// scope.cpp

#include <assert.h>
#include "scope.hpp"

Expr get_scope_value_impl(Expr scope, Expr name)
{
    if (cons_p(scope)) {
        Expr value = assoc(name, scope.cons->car);
        return nil_p(value) ? get_scope_value_impl(scope.cons->cdr, name) : value;
    }

    return scope;
}

Expr get_scope_value(const Scope *scope, Expr name)
{
    return get_scope_value_impl(scope->expr, name);
}

Expr set_scope_value_impl(Gc *gc, Expr &scope, Expr name, Expr value)
{
    if (cons_p(scope)) {
        Expr value_cell = assoc(name, scope.cons->car);

        if (!nil_p(value_cell)) {
            /* A binding already exists, mutate it */
            value_cell.cons->cdr = value;

            return scope;
        } else if (nil_p(scope.cons->cdr)) {
            /* We're at the global scope, add a binding, preserving
             * the identity of the environment list "spine" so that
             * closed-over environments see the new value cell */
            scope.cons->car = CONS(gc, CONS(gc, name, value), scope.cons->car);

            return scope;
        } else {
            /* We haven't found a value cell yet, and we're not at
             * global scope, so recurse */
            set_scope_value_impl(gc, scope.cons->cdr, name, value);

            return scope;
        }
    } else {
        /* ??? Should never happen? */
        return CONS(gc,
                    CONS(gc, CONS(gc, name, value), NIL(gc)),
                    scope);
    }
}

Scope create_scope(Gc *gc)
{
    Scope scope = {
        .expr = CONS(gc, NIL(gc), NIL(gc))
    };
    return scope;
}

void set_scope_value(Gc *gc, Scope *scope, Expr name, Expr value)
{
    scope->expr = set_scope_value_impl(gc, scope->expr, name, value);
}

void push_scope_frame(Gc *gc, Scope *scope, Expr vars, Expr args)
{
    assert(gc);
    assert(scope);

    Expr frame = NIL(gc);

    while (!nil_p(vars) && !nil_p(args)) {
        frame = CONS(gc,
                     CONS(gc, vars.cons->car, args.cons->car),
                     frame);
        vars = vars.cons->cdr;
        args = args.cons->cdr;
    }

    scope->expr = CONS(gc, frame, scope->expr);
}

void pop_scope_frame(Gc *gc, Scope *scope)
{
    assert(gc);
    assert(scope);

    if (!nil_p(scope->expr)) {
        scope->expr = scope->expr.cons->cdr;
    }
}


