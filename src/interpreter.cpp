// interpreter.cpp

#pragma once

#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdarg>
#include <cstdbool>

#include "interpreter.hpp"

/*
* Problem: Evaluation of expressions.

    Evaluation, in the context of programming or scripting languages,
    refers to the process of interpreting expressions represented in
    the language to produce a result or to perform specified actions.
    This process may include arithmetic operations, function invocations,
    and control flow structures among others.

* Solution:
    The code snippet below is part of an interpreter implementation that
    handles the evaluation of expressions. It comprises:

    1) Two helper functions:
       - eval_success: Constructs a successful evaluation result.
       - eval_failure: Constructs a failed evaluation result, typically with an error message or code.

    2) Error handling:
       - These utility functions are designed to simplify the process of signaling success or errors
         when evaluating expressions. Errors may include type mismatches, incorrect argument counts,
         unimplemented functions, or syntax reading issues.

    3) Evaluation of Atoms and Arguments:
       - Although not explicitly shown in this snippet, evaluation would typically involve handling atomic
         values (like numbers, strings) and arguments passed to functions or operations.

    4) Function invocations and Lambda calls:
       - The evaluation mechanism also supports invoking predefined or user-defined functions
         and executing lambda expressions, which are functions defined without a specific name.

    5) Block evaluation:
       - The interpreter may also need to evaluate blocks of expressions, especially for control
         flow structures like if-else conditions, loops, etc.

    6) List Handling:
       - Given that many interpretive languages have list structures, handling the evaluation of lists
         and operations on them (like map, reduce) would also be a part of the evaluation process.

    The following helper functions specifically handle various types of errors that may occur during the evaluation:
*/

// Constructs a result indicating successful evaluation.
EvalResult eval_success(Expr expr)
{
    EvalResult result = {
        .is_error = false,
        .expr = expr,
    };

    return result;
}

// Constructs a result indicating failed evaluation with an error.
EvalResult eval_failure(Expr error)
{
    EvalResult result = {
        .is_error = true,
        .expr = error,
    };

    return result;
}

// Returns an evaluation failure due to receiving an argument of the wrong type.
EvalResult wrong_argument_type(Gc* gc, const std::string& type, Expr obj)
{
    return eval_failure(
        list(gc, "qqe", "wrong-argument-type", type, obj));
}

// Returns an evaluation failure due to receiving an incorrect number of arguments. 
EvalResult wrong_integer_of_arguments(Gc* gc, long int count)
{
    return eval_failure(
        CONS(gc,
            SYMBOL(gc, "wrong-integer-of-arguments"),
            INTEGER(gc, count)));
}

// Returns an evaluation failure indicating that a requested functionality is not implemented.
EvalResult not_implemented(Gc* gc)
{
    return eval_failure(SYMBOL(gc, "not-implemented"));
}

// Returns an evaluation failure due to encountering a read error, with an error message and character index.
EvalResult read_error(Gc* gc, const std::string& error_message, long int character)
{
    return eval_failure(
        list(gc, "qsd", "read-error", error_message, character));
}
/*
* Evaluates atomic expressions 
    (e.g., integers, real numbers, strings, lambda expressions, native functions, and symbols).
    
    If the atom is a symbol, it looks up its value in the given scope.

    If the symbol is not defined in the scope, 
    it returns an error indicating an undefined (void) variable.
*/
EvalResult eval_atom(Gc *gc, Scope *scope, Atom *atom)
{
    (void) scope;
    (void) gc;

    switch (atom->type) {
    case ATOM_INTEGER:
    case ATOM_REAL:
    case ATOM_STRING:
    case ATOM_LAMBDA:
    case ATOM_NATIVE: {
        return eval_success(atom_as_expr(atom));
    }

    case ATOM_SYMBOL: {
        Expr value = get_scope_value(scope, atom_as_expr(atom));

        if (nil_p(value)) {
            return eval_failure(CONS(gc,
                                     SYMBOL(gc, "void-variable"),
                                     atom_as_expr(atom)));
        }

        return eval_success(value.cons->cdr);
    }
    }

    return eval_failure(CONS(gc,
                             SYMBOL(gc, "unexpected-expression"),
                             atom_as_expr(atom)));
}

/*
* Recursively evaluates a list of arguments (expressions). 
    It calls itself to evaluate each argument in the list 
    until all arguments have been successfully evaluated or an error occurs.
*/
EvalResult eval_all_args(Gc *gc, Scope *scope, Expr args)
{
    (void) scope;
    (void) args;

    switch(args.type) {
    case EXPR_ATOM:
        return eval_atom(gc, scope, args.atom);

    case EXPR_CONS: {
        EvalResult car = eval(gc, scope, args.cons->car);
        if (car.is_error) {
            return car;
        }

        EvalResult cdr = eval_all_args(gc, scope, args.cons->cdr);
        if (cdr.is_error) {
            return cdr;
        }

        return eval_success(cons_as_expr(create_cons(gc, car.expr, cdr.expr)));
    }

    default: {}
    }

    return eval_failure(CONS(gc,
                             SYMBOL(gc, "unexpected-expression"),
                             args));
}


/*
* Carries out the invocation of a lambda function. 
    
    It ensures that the lambda receives the correct number of arguments 
    and sets up a new scope for the lambda execution, 
    inheriting the environment from the lambda definition. 
    
    It then sequentially evaluates each expression in the lambda body within this scope.
*/
EvalResult call_lambda(Gc *gc,
                       Expr lambda,
                       Expr args) {
    if (!lambda_p(lambda)) {
        return eval_failure(CONS(gc,
                                 SYMBOL(gc, "expected-callable"),
                                 lambda));
    }

    if (!list_p(args)) {
        return eval_failure(CONS(gc,
                                 SYMBOL(gc, "expected-arguments"),
                                 args));
    }

    Expr vars = lambda.atom->lambda.args_list;

    if (length_of_list(args) != length_of_list(vars)) {
        return eval_failure(CONS(gc,
                                 SYMBOL(gc, "wrong-integer-of-arguments"),
                                 INTEGER(gc, length_of_list(args))));
    }

    Scope scope = {
        .expr = lambda.atom->lambda.envir
    };
    push_scope_frame(gc, &scope, vars, args);

    EvalResult result = eval_success(NIL(gc));

    Expr body = lambda.atom->lambda.body;

    while (!nil_p(body)) {
        result = eval(gc, &scope, body.cons->car);
        if (result.is_error) {
            return result;
        }
        body = body.cons->cdr;
    }

    return result;
}

/*
* Evaluates a function call. 

    First, it evaluates the callable expression to get the function to be called. 
    Then, it conditionally evaluates the arguments 
    (unless it's a special form, in which case arguments are passed unevaluated).
    
    Depending on whether the callable is a native function 
    (implemented in the host language, in this case, C++) or a lambda (user-defined function), 
    
    it delegates the call to the appropriate handler.
*/
EvalResult eval_funcall(Gc *gc,
                        Scope *scope,
                        Expr callable_expr,
                        Expr args_expr) {
    EvalResult callable_result = eval(gc, scope, callable_expr);
    if (callable_result.is_error) {
        return callable_result;
    }

    EvalResult args_result = symbol_p(callable_expr) && is_special(callable_expr.atom->sym)
        ? eval_success(args_expr)
        : eval_all_args(gc, scope, args_expr);

    if (args_result.is_error) {
        return args_result;
    }

    if (callable_result.expr.type == EXPR_ATOM &&
        callable_result.expr.atom->type == ATOM_NATIVE) {
        return ((NativeFunction)callable_result.expr.atom->native.fun)(
            callable_result.expr.atom->native.param, gc, scope, args_result.expr);
    }

    return call_lambda(gc, callable_result.expr, args_result.expr);
}

/*
* This function processes blocks of expressions, 
    typically found in constructs such as (begin ...) or (progn ...). 
    
    It sequentially evaluates each expression in the list and returns the result of the last expression. 
    
    This behavior is typical in Lisp-like languages where blocks of code are executed sequentially, 
    and the value of the block is the value of the last expression evaluated.
*/
EvalResult eval_block(Gc *gc, Scope *scope, Expr block)
{
    assert(gc);
    assert(scope);

    if (!list_p(block)) {
        return wrong_argument_type(gc, "listp", block);
    }

    Expr head = block;
    EvalResult eval_result = eval_success(NIL(gc));

    while (cons_p(head)) {
        eval_result = eval(gc, scope, head.cons->car);
        if (eval_result.is_error) {
            return eval_result;
        }

        head = head.cons->cdr;
    }

    return eval_result;
}

// Evaluates an expression in a given scope.
EvalResult eval(Gc *gc, Scope *scope, Expr expr)
{
    switch(expr.type) {
    case EXPR_ATOM:
        return eval_atom(gc, scope, expr.atom);

    case EXPR_CONS:
        return eval_funcall(gc, scope, expr.cons->car, expr.cons->cdr);

    default: {}
    }

    return eval_failure(CONS(gc,
                             SYMBOL(gc, "unexpected-expression"),
                             expr));
}

/*
* These functions provide basic list manipulation capabilities, essential in a Lisp interpreter.
    car retrieves the first element of a list, corresponding to Lisp's car functionality. 
    
    These operations are fundamental in traversing and manipulating s-expressions.
*/
EvalResult car(Gc *gc, Scope *scope, Expr args)
{
    assert(gc);
    assert(scope);

    Expr xs = NIL(gc);

    EvalResult result = match_list(gc, "e", args, xs);
    if (result.is_error) {
        return result;
    }

    if (nil_p(xs)) {
        return eval_success(xs);
    }

    if (!cons_p(xs)) {
        return wrong_argument_type(gc, "consp", xs);
    }

    return eval_success(CAR(xs));
}

// Matches a list expression against a given format.
EvalResult match_list(Gc* gc, const char* format, Expr xs, ...) {
    va_list args_list;
    va_start(args_list, xs);

    long int i = 0;
    for (;*format != 0 && !nil_p(xs);) {
        if (!cons_p(xs)) {
            va_end(args_list);
            return wrong_argument_type(gc, "consp", xs);
        }

        Expr x = CAR(xs);

        switch (*format) {
        case 'd': {
            if (!integer_p(x)) {
                va_end(args_list);
                return wrong_argument_type(gc, "integerp", x);
            }

            long int* p = va_arg<long int*>(args_list, long int*);
            if (p != NULL) {
                *p = x.atom->num;
            }
        } break;

        case 'f': {
            if (!real_p(x)) {
                va_end(args_list);
                return wrong_argument_type(gc, "realp", x);
            }

            double* p = va_arg<double*>(args_list, double*);
            if (p != NULL) {
                *p = x.atom->real;
            }
        } break;

        case 's': {
            if (!string_p(x)) {
                va_end(args_list);
                return wrong_argument_type(gc, "stringp", x);
            }

            const char** p = va_arg<const char**>(args_list, const char**);
            if (p != NULL) {
                *p = x.atom->str;
            }
        } break;

        case 'q': {
            if (!symbol_p(x)) {
                va_end(args_list);
                return wrong_argument_type(gc, "symbolp", x);
            }

            const char** p = va_arg<const char**>(args_list, const char**);
            if (p != NULL) {
                *p = x.atom->sym;
            }
        } break;

        case 'e': {
            Expr* p = va_arg<Expr*>(args_list, Expr*);
            *p = x;
        } break;

        case '*': {
            Expr* p = va_arg<Expr*>(args_list, Expr*);
            if (p != NULL) {
                *p = xs;
            }
            xs = NIL(gc);
        } break;
        }

        format++;
        if (!nil_p(xs)) {
            xs = CDR(xs);
        }
    }

    if (*format == '*' && nil_p(xs)) {
        Expr* p = va_arg<Expr*>(args_list, Expr*);
        if (p != NULL) {
            *p = NIL(gc);
        }
        format++;
    }

    if (*format != 0 || !nil_p(xs)) {
        va_end(args_list);
        return wrong_integer_of_arguments(gc, i);
    }

    va_end(args_list);
    return eval_success(NIL(gc));
}

