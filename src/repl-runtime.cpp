// repl-runtime.cpp

/*
* The general purpose of this code is to augment a REPL (Read-Eval-Print Loop) 
    environment for a Lisp-like interpreter with specific native functions 
    that enhance its functionality and interactivity.
*/


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

/*
* Ensures the garbage collector and current scope are valid before inspecting the GC.
    Intended to be used as a diagnostic tool to provide insights into the state of the garbage collector.
*/
static EvalResult gcInspectAdapter(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) param;
    (void) args;

    gc_inspect(_gc);

    return eval_success(NIL(_gc));
}

/*
* Introduces a native function that allows the program to exit gracefully when invoked. 
    This function can be called from within the Lisp environment to terminate the REPL session.
*/
static EvalResult quit(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) args;
    (void) param;

    exit(0);

    return eval_success(NIL(_gc));
}

/*
* Provides a mechanism to retrieve and represent the current scope as an expression, 
    allowing for introspection of the current lexical environment.
*/
static EvalResult getScope(void *param, Gc *_gc, Scope *_scope, Expr args)
{
    assert(_gc);
    assert(_scope);
    (void) param;
    (void) args;

    return eval_success(_scope->expr);
}

/*
* Implements a native function for outputting text to the console, 
    supporting basic interactivity and output operations within the REPL environment.
*/
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

/*
* Registers the above-defined native functions into the global scope of the REPL environment, 
    making them accessible for calls from within the Lisp programs being interpreted. 
    
    This setup phase is critical for extending the built-in functionalities available in the REPL.
*/
void load_repl_runtime(Gc *_gc, Scope *_scope)
{
    gc = _gc;
    scope = _scope;

    set_scope_value(gc, scope, SYMBOL(gc, "quit"), NATIVE(gc, quit, nullptr));
    set_scope_value(gc, scope, SYMBOL(gc, "gc-inspect"), NATIVE(gc, gcInspectAdapter, nullptr));
    set_scope_value(gc, scope, SYMBOL(gc, "scope"), NATIVE(gc, getScope, nullptr));
    set_scope_value(gc, scope, SYMBOL(gc, "print"), NATIVE(gc, print, nullptr));
}

