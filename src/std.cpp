#pragma once

#include <assert.h>
#include <string.h>
#include <vector>

#include "gc.hpp"
#include "interpreter.hpp"
#include "builtins.hpp"
#include "scope.hpp"
#include "parser.hpp"

#include "std.hpp"

struct EvalResult {
    bool is_error;
    Expr expr;
};

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

struct LambdaFn {
    Gc* gc;
    Expr args;
    Expr body;
    Scope* scope;

    Expr operator()() {
        return atom_as_expr(create_lambda_atom(gc, args, body, scope->expr));
    }
};

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

struct ListOpFn {
    Gc* gc;
    Scope* scope;

    EvalResult operator()(Expr args) {
        assert(gc);
        assert(scope);

        return eval_success(args);
    }
};

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


void load_std_library(Gc* gc, Scope* scope) {
    set_scope_value(gc, scope, SYMBOL(gc, "car"), NATIVE(gc, car, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, ">"), NATIVE(gc, greaterThan, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "+"), NATIVE(gc, plus_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "*"), NATIVE(gc, mul_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "list"), NATIVE(gc, list_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "t"), SYMBOL(gc, "t"));
    set_scope_value(gc, scope, SYMBOL(gc, "nil"), SYMBOL(gc, "nil"));
    set_scope_value(gc, scope, SYMBOL(gc, "assoc"), NATIVE(gc, assoc_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "quasiquote"), NATIVE(gc, quasiquote, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "set"), NATIVE(gc, set, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "quote"), NATIVE(gc, quote, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "begin"), NATIVE(gc, begin, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "defun"), NATIVE(gc, defun, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "when"), NATIVE(gc, when, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "lambda"), NATIVE(gc, lambda_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "λ"), NATIVE(gc, lambda_op, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "unquote"), NATIVE(gc, unquote, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "load"), NATIVE(gc, load, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "append"), NATIVE(gc, append, NULL));
    set_scope_value(gc, scope, SYMBOL(gc, "equal"), NATIVE(gc, equal_op, NULL));
}