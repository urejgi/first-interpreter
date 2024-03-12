// gc.cpp

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

#include "builtins.hpp"
#include "expr.hpp"
#include "gc.hpp"

#define GC_INITIAL_CAPACITY 256



/* 
Using static in this context means that the function has internal linkage.
Internal linkage means that the function is only visible within the 
translation unit(source file) it is defined in, and not in other source 
files that include the header file.

This is useful to avoid potential name clashes with other 
functions of the same name in other source files.
*/


/*
* ### Auxiliary Functions:
    - value_of_expr and compare_exprs: 
        Utility functions supporting gc_find_expr. 
        They facilitate comparison between expressions 
        by mapping them to their address representations. 
        
        This is a technique used to enable searching and sorting operations 
        on complex structures through simple integer comparisons.
*/


// Returns the integer representation of an expression.
static intptr_t value_of_expr(const Expr& expr)
{
    if (expr->type == EXPR_CONS) {
        return reinterpret_cast<intptr_t>(expr->cons.get());
    } else if (expr->type == EXPR_ATOM) {
        return reinterpret_cast<intptr_t>(expr->atom.get());
    } else {
        return 0;
    }
}

// Compares two expressions based on their integer representations. 
static int compare_exprs(const void *a, const void *b)
{
    assert(a);
    assert(b);

    const intptr_t ptr_a = value_of_expr(*static_cast<const std::unique_ptr<Expr>&>(*static_cast<const std::unique_ptr<Expr>*>(a)));
    const intptr_t ptr_b = value_of_expr(*static_cast<const std::unique_ptr<Expr>&>(*static_cast<const std::unique_ptr<Expr>*>(b)));
    const intptr_t d = ptr_b - ptr_a;

    if (d < 0) {
        return -1;
    } else if (d > 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
* Allocates memory for a new Gc instance and initializes its members, 
    particularly setting up initial capacities for the expression vector and visited marker vector. 

    If allocation fails, it safely cleans up before returning nullptr.
*/
Gc *create_gc()
{
    Gc *gc = new Gc();
    if (gc == nullptr) {
        goto error;
    }

    gc->exprs.reserve(GC_INITIAL_CAPACITY);
    gc->visited.resize(GC_INITIAL_CAPACITY, false);

    gc->size = 0;
    gc->capacity = GC_INITIAL_CAPACITY;

    return gc;

error:
    destroy_gc(gc);
    return nullptr;
}

/*
*  Iterates over the tracked expressions to explicitly call on each 
    before deallocating the Gc instance itself. 
    
    This step is crucial for properly freeing any custom-managed memory inside each Expr.
*/
void destroy_gc(Gc *gc)
{
    assert(gc);

    for (size_t i = 0; i < gc->size; ++i) {
        destroy_expr(gc->exprs[i].get());
    }

    if (gc) {
        delete gc;
    }
}

/*
* Adds a new Expr to the garbage collector's tracking list.
    It ensures that there's adequate capacity in the GC's structures, resizing if necessary, 
    and then takes ownership of the expression using std::move to avoid unnecessary copying.
*/
int gc_add_expr(Gc *gc, Expr expr)
{
    assert(gc);

    if (gc->size >= gc->capacity) {
        const size_t new_capacity = gc->capacity * 2;
        std::vector<std::unique_ptr<Expr>> new_exprs(new_capacity);
        std::vector<bool> new_visited(new_capacity, false);

        for (size_t i = 0; i < gc->size; ++i) {
            new_exprs[i] = std::move(gc->exprs[i]);
        }

        gc->capacity = new_capacity;
        gc->exprs = std::move(new_exprs);
        gc->visited = std::move(new_visited);
    }

    gc->exprs[gc->size++] = std::move(expr);

    return 0;
}

/*
*  Locates a given expression within the GC's tracking list using binary search (bsearch).
    The search is based on the integer representation of the expression addresses,
    which allows quickly determining if an expression is managed by the GC. 
    
    This could be part of an optimization to prevent adding duplicates 
    or for finding expressions during the mark phase of a garbage collection cycle.
*/
static long int gc_find_expr(Gc *gc, const Expr& expr)
{
    assert(gc);

    Expr *result =
        static_cast<Expr*>(bsearch(&expr, gc->exprs.data(), gc->size,
                                sizeof(Expr), compare_exprs));

    if (result == nullptr) {
        return -1;
    }

    return (long int) (result - gc->exprs.data());
}



// Methodology.
// 
// Problem: memory management in an environment where expressions are dynamically created 
// and may become unreachable
//  (i.e., no references point to them anymore), which would otherwise result in memory leaks. 
// 
// Solution: implementation of simplified Mark-and-Sweep algorithm.
// 
// Description of the solution:
// 
// ### Mark-and-Sweep Garbage Collection.
/*
* Garbage collection here is performed in two major phases: marking and sweeping.

1. Mark Phase (gc_traverse_expr): 
    
    This function recursively traverses the expression graph
        starting from a given root Expr. It marks each expression encountered 
            as visited (gc->visited[root_index] = true;) to indicate it's still in use.
        
        This traversal accounts for different types of expressions, 
            including cons cells and lambda atoms, 
                ensuring all reachable expressions are marked. 
        
        This process identifies all expressions that are still "alive" 
            or reachable from the root,
                    leveraging the ability to navigate through 
                        the expression structure to find all connected expressions.

2. Sweep Phase (gc_collect): 
    
    - First, the expression list (gc->exprs) is sorted and defragmented 
        to ensure that all void expressions (unused slots) are removed,
            effectively compacting the list of tracked expressions.
    
    - It then initializes the visited marker for each expression to false, 
        preparing for the mark phase.
    
    - The mark phase is initiated by calling gc_traverse_expr(gc, root), 
        which marks all reachable (alive) expressions from the provided root.
    
    - After marking, the code iterates through all expressions, 
        deallocating those not marked as visited 
        (i.e., those that are unreachable and thus can safely be cleaned up) 
            by replacing them with an EXPR_VOID type expression to indicate the slot is now unused.


3. Conclusion:
    The mark-and-sweep approach implemented here efficiently identifies and deallocates unreachable expressions
    in a system possibly involving complex relationships between dynamic expressions. 
    
    By marking expressions reachable from a known root before sweeping, 
    the garbage collector minimizes the risk of prematurely freeing memory still in use 
    (dangling pointer problem) or memory leaks (unreachable memory not being freed).
    
    This garbage collection solution is suitable for managing memory in environments
    like interpreters or symbolic computation systems where the memory footprint of expressions 
    can grow and shrink dynamically, 
    requiring a flexible and robust method to ensure memory efficiency and program stability.

*/



// Performs a depth-first traversal of an expression tree.
static void gc_traverse_expr(Gc *gc, const Expr& root)
{
    assert(gc);
    assert(root->type != EXPR_VOID);
    const long int root_index = gc_find_expr(gc, root);
    if (root_index < 0) {
        std::cerr << "GC tried to collect something that was not registered" << std::endl;
        print_expr_as_sexpr(std::cerr, root);
        std::cerr << std::endl;
        assert(root_index >= 0);
    }

    if (gc->visited[root_index]) {
        return;
    }

    gc->visited[root_index] = true;

    if (cons_p(root)) {
        gc_traverse_expr(gc, root->cons->car);
        gc_traverse_expr(gc, root->cons->cdr);
    } else if (root->type == EXPR_ATOM
               && root->atom->type == ATOM_LAMBDA) {
        gc_traverse_expr(gc, root->atom->lambda.args_list);
        gc_traverse_expr(gc, root->atom->lambda.body);
        gc_traverse_expr(gc, root->atom->lambda.envir);
    }
}

// Performs garbage collection on the GC's list of expressions.
void gc_collect(Gc *gc, const Expr& root)
{
    assert(gc);

    // Sort gc->exprs O(nlogn)
    std::sort(gc->exprs.begin(), gc->exprs.begin() + gc->size,
              [](const std::unique_ptr<Expr>& a, const std::unique_ptr<Expr>& b) {
                  return value_of_expr(*a) < value_of_expr(*b);
              });

    // Defragment O(n)
    gc->exprs.erase(std::remove_if(gc->exprs.begin(), gc->exprs.begin() + gc->size,
                                   [](const std::unique_ptr<Expr>& expr) {
                                       return expr->type == EXPR_VOID;
                                   }), gc->exprs.end());
    gc->size = gc->exprs.size();

    // Initialize visited array O(n)
    std::fill(gc->visited.begin(), gc->visited.begin() + gc->size, false);

    // Traverse root O(nlogn)
    gc_traverse_expr(gc, root);

    // Dealloc unvisited O(n)
    for (size_t i = 0; i < gc->size; ++i) {
        if (!gc->visited[i]) {
            destroy_expr(gc->exprs[i].get());
            gc->exprs[i] = std::unique_ptr<Expr>(new Expr(EXPR_VOID));
        }
    }
}

// Prints a visual representation of the GC's list of expressions. 
void gc_inspect(const Gc *gc)
{
    for (size_t i = 0; i < gc->size; ++i) {
        if (gc->exprs[i]->type == EXPR_VOID) {
            std::cout << ".";
        } else {
            std::cout << "+";
        }
    }
    std::cout << std::endl;
}

