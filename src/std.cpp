#pragma once

#include <assert.h>
#include <string.h>
#include <vector>

#include "std.hpp"

/*
*Primary functionalities: 
    type conversion, lambda function creation, quasiquote handling,
    checking if an expression is sorted, and error handling for misused expressions.

    Check the comments to the functions for more details.
*/

// Represents the result of an evaluation, which can be an error or a value.
struct EvalResult {
    bool is_error;
    Expr expr;
};

/*
* This structure is responsible for converting numeric expressions to real numbers.
    If the expression is already a real number, it returns the expression unchanged.
    If it's an integer, it converts it to a real number. 
    If the expression is neither, it reports an error indicating a wrong argument type.
*/
struct RealFn {
    Gc* gc;
    Expr operator()(Expr a) {
        if (real_p(a)) {
            return eval_success(a);
        }

        if (integer_p(a)) {
            return eval_success(REAL(gc, (float)a.atom->num));
        }

        return wrong_argument_type(gc, "(or realp integerp)", a);
    }
};

/*
*  This structure creates a lambda function, 
    encapsulating a piece of code ("body") that can be executed later with 
    a given set of arguments ("args") within a specific lexical scope. 
*/
struct LambdaFn {
    Gc* gc;
    Expr args;
    Expr body;
    Scope* scope;

    Expr operator()() {
        return atom_as_expr(create_lambda_atom(gc, args, body, scope->expr));
    }
};

/*
* A quasiquote expression is used to include unevaluated 
  expressions within a list structure, which will be evaluated 
  when the list is evaluated.
  
  Example: (1 ,(+ 2 3) 4)
*/

/*
* Quasiquote expressions allow parts of a code list to be evaluated only when 
    the list itself is evaluated.

    This facilitates writing macros or generating code on the fly. 
    
    The QuasiquoteFn struct manages these expressions, evaluating them as needed,
    handling both quoted and unquoted parts within a quasiquote.
*/
struct QuasiquoteFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        Expr expr = void_expr();
        EvalResult result = match_list(gc, "e", args, &expr);
        if (result.is_error) {
            return result;
        }

        const char* unquote = NULL;
        Expr unquote_expr = void_expr();
        result = match_list(gc, "qe", expr, &unquote, &unquote_expr);

        if (!result.is_error && strcmp(unquote, "unquote") == 0) {
            return eval(gc, scope, unquote_expr);
        }
        else if (cons_p(expr)) {
            EvalResult left = quasiquote(*this, gc, scope, CONS(gc, CAR(expr), NIL(gc)));
            if (left.is_error) {
                return left;
            }
            EvalResult right = quasiquote(*this, gc, scope, CONS(gc, CDR(expr), NIL(gc)));
            if (right.is_error) {
                return right;
            }
            return eval_success(CONS(gc, left.expr, right.expr));
        }
        else {
            return eval_success(expr);
        }
    }
};

/*
* This structure represents a function call that's invalid in the context outside of a quasiquote expression. 
    
    Its purpose is to catch and report errors when unquote is misused, not nested within a quasiquote.
*/
struct UnquoteFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        assert(gc);
        assert(scope);
        (void)args;

        return eval_failure(STRING(gc, "Using unquote outside of quasiquote."));
    }
};

/*
* This struct checks if one number is greater than another. 
    It's flexible enough to handle both integers and real numbers by converting integers to reals when necessary.
    This functionality is crucial in expressions where comparative logic is applied.
*/
struct GreaterThan2Fn {
    Gc* gc;

    EvalResult operator()(Expr a, Expr b) {
        if (integer_p(a) && integer_p(b)) {
            return eval_success(bool_as_expr(gc, a.atom->num > b.atom->num));
        }
        else {
            EvalResult result_a = real(*this, gc, a);
            if (result_a.is_error) return result_a;

            EvalResult result_b = real(*this, gc, b);
            if (result_b.is_error) return result_b;

            return eval_success(
                bool_as_expr(gc, result_a.expr.atom->real > result_b.expr.atom->real));
        }
    }
};

/*
* This is an extension of the previous function to handle lists of numbers, 
    checking whether the list is sorted in ascending order. 
    
    It iterates through the list, comparing each pair of adjacent elements 
    to ensure that each is greater than or equal to the previous. 
    
    This could be used for assertions or validations within the Lisp program.
*/
struct GreaterThanFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        assert(gc);
        assert(scope);

        if (!cons_p(args)) {
            return wrong_argument_type(gc, "consp", args);
        }

        Expr x1 = CAR(args);
        args = CDR(args);

        bool sorted = true;
        while (!nil_p(args) && sorted) {
            Expr x2 = CAR(args);
            args = CDR(args);

            EvalResult result = greaterThan2(*this, gc, x1, x2);
            if (result.is_error) {
                return result;
            }

            sorted = sorted && !nil_p(result.expr);

            x1 = x2;
        }

        return eval_success(bool_as_expr(gc, sorted));
    }
};

/*
*  A function intended to apply a given operation to each element in a list, 
    though currently implemented as a simple pass-through, 
    returning the provided arguments without modification.
*/
struct ListOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        assert(gc);
        assert(scope);

        return eval_success(args);
    }
};

/*
* Defines a method for adding two numeric expressions, handling both integer and real types.
    It checks the type of the inputs and performs the addition accordingly.
*/
struct Plus2Fn {
    Gc* gc;

    EvalResult operator()(Expr a, Expr b) {
        if (integer_p(a) && integer_p(b)) {
            return eval_success(INTEGER(gc, a.atom->num + b.atom->num));
        }
        else {
            EvalResult result_a = real(*this, gc, a);
            if (result_a.is_error) return result_a;

            EvalResult result_b = real(*this, gc, b);
            if (result_b.is_error) return result_b;

            return eval_success(
                REAL(gc, result_a.expr.atom->real + result_b.expr.atom->real));
        }
    }
};

/*
* Iterates through a given list of numeric expressions, summing them up.
    This function checks to ensure that the list's structure is correct 
    (each element is part of a list) and then accumulates their sum.
*/
struct PlusOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr acc = INTEGER(gc, 0L);

        while (!nil_p(args)) {
            if (!cons_p(args)) {
                return wrong_argument_type(gc, "consp", args);
            }

            Expr car = CAR(args);

            EvalResult result = plus2(*this, gc, acc, car);

            if (result.is_error) {
                return result;
            }

            acc = result.expr;
            args = CDR(args);
        }

        return eval_success(acc);
    }
};

/*
* Defines an operation to multiply two numeric expressions, 
    handling both integers and real numbers by conducting type-specific arithmetic operations. 
    It offers flexibility to accommodate dynamic type evaluation.
*/
struct Mul2Fn {
    Gc* gc;

    EvalResult operator()(Expr a, Expr b) {
        if (integer_p(a) && integer_p(b)) {
            return eval_success(INTEGER(gc, a.atom->num * b.atom->num));
        }
        else {
            EvalResult result_a = real(*this, gc, a);
            if (result_a.is_error) return result_a;

            EvalResult result_b = real(*this, gc, b);
            if (result_b.is_error) return result_b;

            return eval_success(
                REAL(gc, result_a.expr.atom->real * result_b.expr.atom->real));
        }
    }
};

/*
* Iterates over each element in a list, applying a multiplication operation to aggregate a product. 
    This structure ensures the structure of the list is valid for operations 
    and uses an accumulator to maintain the ongoing product.
*/
struct MulOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr acc = INTEGER(gc, 1);

        while (!nil_p(args)) {
            if (!cons_p(args)) {
                return wrong_argument_type(gc, "consp", args);
            }

            Expr car = CAR(args);

            EvalResult result = mul2(*this, gc, acc, car);

            if (result.is_error) {
                return result;
            }

            acc = result.expr;
            args = CDR(args);
        }

        return eval_success(acc);
    }
};

/*
* Searches for a given key within an association list, returning the first matching key-value pair.
    This function is useful for retrieving data from structured lists that represent key-value pairs.
*/
struct AssocOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr key = NIL(gc);
        Expr alist = NIL(gc);
        EvalResult result = match_list(gc, "ee", args, &key, &alist);
        if (result.is_error) {
            return result;
        }

        return eval_success(assoc(key, alist));
    }
};

/*
* Updates or sets the value of a variable within a given scope. 
    It performs an evaluation of the provided expression before setting the variable's value.
*/
struct SetFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        const char* name = NULL;
        Expr value = void_expr();
        EvalResult result = match_list(gc, "qe", args, &name, &value);
        if (result.is_error) {
            return result;
        }

        result = eval(gc, scope, value);
        if (result.is_error) {
            return result;
        }

        set_scope_value(gc, scope, SYMBOL(gc, name), result.expr);

        return eval_success(result.expr);
    }
};

/*
*  Takes an expression and returns it unevaluated. 
    This function embodies the concept of quoting, 
    which allows for treating code as data without immediate execution,
    a fundamental aspect of metaprogramming and symbolic computation in Lisp.
*/
struct QuoteFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr expr = void_expr();
        EvalResult result = match_list(gc, "e", args, &expr);
        if (result.is_error) {
            return result;
        }

        return eval_success(expr);
    }
};

/*
*  Evaluates a block (sequence) of expressions in order, returning the result of the last expression.
    This function enables grouping of expressions, 
    useful in scenarios where multiple steps or operations need to be executed sequentially.
*/
struct BeginFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr block = void_expr();
        EvalResult result = match_list(gc, "*", args, &block);
        if (result.is_error) {
            return result;
        }

        return eval_block(gc, scope, block);
    }
};

/*
* Defines a new function within the current scope.
* It takes three parameters: a name for the function, a list of argument names, and the function body.
* This enables users to create custom functions that can be called elsewhere in the program.
*/

struct DefunFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr name = void_expr();
        Expr args_list = void_expr();
        Expr body = void_expr();

        EvalResult result = match_list(gc, "ee*", args, &name, &args_list, &body);
        if (result.is_error) {
            return result;
        }

        if (!list_of_symbols_p(args_list)) {
            return wrong_argument_type(gc, "list-of-symbolsp", args_list);
        }

        return eval(gc, scope,
            list(gc, "qee", "set", name,
                lambda(gc, args_list, body, scope)));
    }
};

/*
* Represents a conditional operation similar to 'if' statements in other programming languages.
* It evaluates a given condition; if the condition is true (non-nil), it executes a block of code.
* Useful for controlling the flow of execution based on specific conditions.
*/
struct WhenFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr condition = void_expr();
        Expr body = void_expr();

        EvalResult result = match_list(
            gc, "e*", args, &condition, &body);
        if (result.is_error) {
            return result;
        }

        result = eval(gc, scope, condition);
        if (result.is_error) {
            return result;
        }

        if (!nil_p(result.expr)) {
            return eval_block(gc, scope, body);
        }

        return eval_success(NIL(gc));
    }
};

/*
* Creates an anonymous function (lambda) with a specified list of parameters and a body.
* Lambda functions are powerful constructs for scenarios requiring function objects, 
    closures, or higher-order functions.
* The created lambda function can be passed as an argument, returned from other functions, or bound to a variable.
*/
struct LambdaOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr args_list = void_expr();
        Expr body = void_expr();

        EvalResult result = match_list(gc, "e*", args, &args_list, &body);
        if (result.is_error) {
            return result;
        }

        if (!list_of_symbols_p(args_list)) {
            return wrong_argument_type(gc, "list-of-symbolsp", args_list);
        }

        return eval_success(lambda(gc, args_list, body, scope));
    }
};

/*
* Checks for equality between two expressions.
* It evaluates to a truthy value if both expressions are equal, 
    according to the defined equality semantics of the language.
*/
struct EqualOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        Expr obj1;  
        Expr obj2;
        EvalResult result = match_list(gc, "ee", args, &obj1, &obj2);
        if (result.is_error) {
            return result;
        }

        if (equal(obj1, obj2)) {
            return eval_success(T(gc));
        }
        else {
            return eval_success(NIL(gc));
        }
    }
};

/*
* Responsible for loading and executing Lisp code from an external file.
* This function parses the file specified by the filename argument, 
    reading all expressions contained within.
* Once parsed, the expressions are sequentially evaluated within the given scope.
*/
struct LoadFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        const char* filename = NULL;
        EvalResult result = match_list(gc, "s", args, &filename);
        if (result.is_error) {
            return result;
        }

        ParseResult parse_result = read_all_exprs_from_file(gc, filename);
        if (parse_result.is_error) {
            // Include the line number and column number in the error message
            return read_error(gc, parse_result.error_message, parse_result.line, parse_result.column);
        }

        return eval_block(gc, scope, parse_result.expr);
    }
};

/*
* Concatenates multiple lists into a single list.
* It takes a list of lists as an argument and merges them, preserving the order.
* This operation is recursive, ensuring that all nested lists are appended in order.
* Handy for operations requiring the combination of multiple sequences into one.
*/
struct AppendFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        (void)gc;
        assert(scope);

        if (nil_p(args)) {
            return eval_success(NIL(gc));
        }

        return append_helper(*this, gc, scope, args);
    }

private:
    EvalResult append_helper(Gc* gc, Scope* scope, Expr xs) {
        if (nil_p(xs)) {
            return eval_success(NIL(gc));
        }

        Expr xs1 = void_expr();
        Expr x = void_expr();
        EvalResult result = match_list(gc, "e*", xs, &x, &xs1);
        if (result.is_error) {
            return result;
        }

        EvalResult result_xs1 = append_helper(gc, scope, xs1);
        if (result_xs1.is_error) {
            return result_xs1;
        }

        return eval_success(CONS(gc, x, result_xs1.expr));
    }
};

/*
* Initializes the Lisp environment with the standard library functions and constants.
* This includes basic arithmetic operations, list manipulation functions, 
    control structures, and special forms.
*/
void load_std_library(Gc* gc, Scope* scope) {
    set_scope_value(gc, scope, SYMBOL(gc, "car"), NATIVE(gc, car, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, ">"), NATIVE(gc, greaterThan, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "+"), NATIVE(gc, plus_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "*"), NATIVE(gc, mul_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "list"), NATIVE(gc, list_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "t"), SYMBOL(gc, "t"));       // ???
    set_scope_value(gc, scope, SYMBOL(gc, "nil"), SYMBOL(gc, "nil"));
    set_scope_value(gc, scope, SYMBOL(gc, "assoc"), NATIVE(gc, assoc_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "quasiquote"), NATIVE(gc, quasiquote, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "set"), NATIVE(gc, set, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "quote"), NATIVE(gc, quote, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "begin"), NATIVE(gc, begin, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "defun"), NATIVE(gc, defun, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "when"), NATIVE(gc, when, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "lambda"), NATIVE(gc, lambda_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "λ"), NATIVE(gc, lambda_op, NULL));       // ???
    set_scope_value(gc, scope, SYMBOL(gc, "unquote"), NATIVE(gc, unquote, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "load"), NATIVE(gc, load, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "append"), NATIVE(gc, append, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "equal"), NATIVE(gc, equal_op, NULL));
}



// Yea, 666 lines of code :)