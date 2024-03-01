// repl-runtime.cpp

#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "gc.hpp"
#include "interpreter.hpp"
#include "expr.hpp"
#include "scope.hpp"
#include "symbol.hpp"

static Gc *gc;
static Scope *scope;

// Native function to inspect Garbage Collector.
static EvalResult gcInspectAdapter(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) param;
    (void) args;

    gc_inspect(_gc);

    return eval_success(NIL(_gc));
}

// Native function to quit the REPL.
static EvalResult quit(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) args;
    (void) param;

    exit(0);

    return eval_success(NIL(_gc));
}

// Native function to get the value of a scope.
static EvalResult getScope(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) param;
    (void) args;

    return eval_success(_scope->expr);
}

// Native function for console output.
static EvalResult print(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) param;

    const std::string&s = nullptr;
    EvalResult result = match_list(_gc, "s", args, &s);
    if (result.is_error) {
        return result;
    }

    std::cout << s << std::endl;

    return eval_success(NIL(_gc));
}

// Native function to load REPL's runtime.
void load_repl_runtime(Gc *_gc, Scope *_scope)
{
    gc = _gc;
    scope = _scope;

    set_scope_value(gc, scope, SYMBOL(gc, "quit"), NATIVE(gc, quit, nullptr));
    set_scope_value(gc, scope, SYMBOL(gc, "gc-inspect"), NATIVE(gc, gcInspectAdapter, nullptr));
    set_scope_value(gc, scope, SYMBOL(gc, "scope"), NATIVE(gc, getScope, nullptr));
    set_scope_value(gc, scope, SYMBOL(gc, "print"), NATIVE(gc, print, nullptr));
}

