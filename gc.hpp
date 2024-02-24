#ifndef GC_H_
#define GC_H_

#include "expr.hpp"

class Gc
{
public:
    Gc() = default;
    ~Gc();

    static Gc* create_gc();
    static void destroy_gc(Gc* gc);

    int add_expr(Expr expr);
    void collect(Expr root);
    void inspect() const;

private:
    // Implementation details
};

#endif  // GC_H_
