// expr.cpp

/*
    expr.hpp provides an implementation of data structure that represents a
    fundamental unit of storage, an S-expression.

    S-expressions are used to represent both 
    simple data atoms (like integers, symbols, or strings) 
    and compound data structures (like lists or trees) 
    in a hierarchical, parenthesized format.

    expr.cpp (current file) provides an implementation of
    funtions to manipulate S-expressions.


*/


#pragma once

#include <assert.h>
#include <stdio.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include "expr.hpp"

// Create an Expr from an Atom.
Expr atom_as_expr(const Atom& atom)
{
    Expr expr = {
        .type = EXPR_ATOM,
        .atom = atom
    };

    return expr;
}

// Create an Expr from a Cons.
Expr cons_as_expr(const Cons& cons)
{
    Expr expr = {
        .type = EXPR_CONS,
        .cons = cons
    };

    return expr;
}

// Create a void Expr.
Expr void_expr(void)
{
    Expr expr = {
        .type = EXPR_VOID
    };

    return expr;
}


/*
* Given an atom, 
    this function prints its representation as an S-expression to a file stream. 

    It handles different atom types by printing symbols as themselves, 
    numbers according to their type, strings with quotes, lambda expressions with a special notation, 
    and native functions with a placeholder.
*/

void print_atom_as_sexpr(FILE *stream, const Atom& atom)
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

/*
*  Prints a cons cell and its descendants as an S-expression.
    It recursively handles nested lists and properly formats the output to resemble traditional Lisp notation, 
    including parentheses and dots for improper lists.
*/

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

/*
* Given an Expr, it prints its representation as an S-expression. 
    It delegates to print_atom_as_sexpr or print_cons_as_sexpr depending on the expression type.
*/

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

/* A cleanup function that deallocates memory used by an Expr.

    For atomic expressions, it releases atom - related resources.
    
    For cons cells, it recursively frees resources used by the car and cdr components.
*/

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

/*
* ### Creation of Atoms and Cons Cells

    Each creation function dynamically allocates memory for a new atom or cons cell, 
    initializes its fields,
    and then registers it with a garbage collector (GC) for managed memory handling.
*/
    

/*
* Takes two Expr objects (car and cdr) to create a new cons cell.
*/
Cons *create_cons(Gc *gc, Expr car, Expr cdr)
{
    Cons *cons = new Cons;
    cons->car = car;
    cons->cdr = cdr;

    gc_add_expr(gc, cons_as_expr(cons));

    return cons;
}

/*
* Frees the memory allocated for a cons cell. 
    This function is straightforward 
    as it does not recursively destroy the expressions pointed to by the car and cdr. 
*/
void destroy_cons(Cons *cons)
{
    delete cons;
}

/*
* - create_real_atom, create_integer_atom, create_string_atom,
    create_symbol_atom, create_lambda_atom, create_native_atom: 

    Each of these functions creates a specific type of atom 
    (real, integer, string, symbol, lambda, and native function, respectively). 

    They set the appropriate type and content for the atom, 
    and then attempt to register the atom with the GC. 

    If registration fails, these functions clean up by deallocating the atom and return NULL, 
    indicating failure.
*/

//  Create a real Atom.
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

// Create an integer Atom.
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

// Create a string Atom.
Atom *create_string_atom(Gc *gc, const std::string& str, const std::string& str_end)
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

// Create a symbol Atom.
Atom *create_symbol_atom(Gc *gc, const std::string& sym, const std::string& sym_end)
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

// Create a lambda Atom.
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

// Create a native Atom.
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

/*
* Frees the memory allocated for an atom.
    For ATOM_SYMBOL and ATOM_STRING, where strings are dynamically allocated, 
    it also frees the memory for the string before freeing the atom itself.
    
    For other atom types (like ATOM_LAMBDA, ATOM_NATIVE, ATOM_INTEGER, and ATOM_REAL),
    there's no extra dynamically allocated memory directly associated with the atom, 
    so it simply deletes the atom.
*/

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

// ### Converting Atoms and Cons to S-expressions String Representation.

/*Description:
*  - atom_as_sexpr and cons_as_sexpr functions aim to serialize Atom and Cons structures 
    respectively into their string representation as S-expressions. 

    These functions use snprintf to format the output string. 
    
    This part of the code involves traversing through the linked list formed by Cons cells if present, 
    and appending the string representation of each atom to an output string.

    - The cons_as_sexpr function is more complex due to the potential for recursively nested structures.
    
    It must account for proper parentheses formatting and handle the special case of improper lists
    (i.e., lists where the final cdr is not nil or another cons cell, indicated by a dot).
*/


// Convert an Atom to an S - expression.
int atom_as_sexpr(Atom *atom, std::string output, size_t n)
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

// Convert an Cons to an S - expression.
int cons_as_sexpr(Cons* head, char* output, size_t n)
{
    assert(head);
    assert(output);

    Cons* cons = head;

    int m = (int)n;

    int c = snprintf(output, n, "(");
    if (m - c <= c) {
        return c;
    }

    c += expr_as_sexpr(cons->car, output + c, (size_t)(m - c));
    if (m - c <= 0) {
        return c;
    }

    while (cons->cdr.type == EXPR_CONS) {
        cons = cons->cdr.cons;

        c += snprintf(output + c, (size_t)(m - c), " ");
        if (m - c <= 0) {
            return c;
        }

        c += expr_as_sexpr(cons->car, output + c, (size_t)(m - c));
        if (m - c <= 0) {
            return c;
        }
    }

    if (cons->cdr.atom->type != ATOM_SYMBOL ||
        strcmp("nil", cons->cdr.atom->sym) != 0) {

        c += snprintf(output + c, (size_t)(m - c), " . ");
        if (m - c <= 0) {
            return c;
        }

        c += expr_as_sexpr(cons->cdr, output + c, (size_t)(m - c));
        if (m - c <= 0) {
            return c;
        }
    }

    c += snprintf(output + c, (size_t)(m - c), ")");
    if (m - c <= 0) {
        return c;
    }

    return c;
}



/*
*  Converges the conversion functionalities for either atom or cons types of expressions into 
    a single interface by delegating to the specific function based on the expression type.
*/

int expr_as_sexpr(Expr expr, std::string output, size_t n)
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

//### Type to String Conversion Utilities.

/*
* - expr_type_as_string and atom_type_as_string functions provide a simple mapping 
    from the internal enum representation of the expression and atom types 
    to human-readable string equivalents. 

    This can be useful for debugging or any functionality that involves type introspection.
*/


// Convert ExprType to a string representation.
const std::string& expr_type_as_string(ExprType expr_type)
{
    switch (expr_type) {
    case EXPR_ATOM: return "EXPR_ATOM";
    case EXPR_CONS: return "EXPR_CONS";
    case EXPR_VOID: return "EXPR_VOID";
    }

    return "";
}

// Correlate Atom type to it's string representation.
const std::string& atom_type_as_string(AtomType atom_type)
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

/*
May be improved like that:
* Usage of const references (const Expr& expr) instead of values (Expr expr).
*  Instead of using raw pointers, consider using smart pointers
    (std::unique_ptr, std::shared_ptr) to manage memory automatically and avoid memory leaks.
*/
