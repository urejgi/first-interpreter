#ifndef GC_H_
#define GC_H_

#pragma once

#include "expr.hpp"

struct Gc {
    std::vector<std::unique_ptr<Expr>> exprs;
    std::vector<bool> visited;
    size_t size;
    size_t capacity;
};


Gc* create_gc();
void destroy_gc(Gc* gc);

int gc_add_expr(Gc* gc, Expr expr);

void gc_collect(Gc* gc, const Expr& root);
void gc_inspect(const Gc* gc);

#endif  // GC_H_
