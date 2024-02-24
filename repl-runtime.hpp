#ifndef REPL_RUNTIME_H_
#define REPL_RUNTIME_H_

#include "gc.hpp"
#include "scope.hpp"

void load_repl_runtime(Gc* gc, Scope* scope);

#endif  // REPL_RUNTIME_H_
