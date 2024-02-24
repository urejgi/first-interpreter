#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>

#include "expr.hpp"

bool equal(const Expr& obj1, const Expr& obj2);

bool isNil(const Expr& obj);
bool isSymbol(const Expr& obj);
bool isString(const Expr& obj);
bool isInteger(const Expr& obj);
bool isReal(const Expr& obj);
bool isCons(const Expr& obj);
bool isList(const Expr& obj);
bool isListOfSymbols(const Expr& obj);
bool isLambda(const Expr& obj);

bool isSpecial(const std::string& name);

long int listLength(const Expr& obj);

Expr assoc(const Expr& key, const Expr& alist);

Expr list(Gc* gc, const std::string& format, ...);

Expr boolAsExpr(Gc* gc, bool condition);

#endif  // BUILTINS_H_