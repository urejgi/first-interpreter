#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdexcept>
#include <string>
#include <system_error>
#include "expr.hpp"

struct Scope;
struct Gc;

structFailure: public std::runtime_error{
  explicit EvalFailure(const std::string & what) : std::runtime_error(what) {}
};

struct WrongArgumentType : public EvalFailure {
    WrongArgumentType(Gc* gc, const std::string& type, const Expr& obj)
        : EvalFailure("Wrong argument type: " + type + ", obj: " + obj.to_string(gc)) {}
};

struct WrongIntegerOfArguments : public EvalFailure {
    WrongIntegerOfArguments(Gc* gc, long int count)
        : EvalFailure("Wrong number of arguments: " + std::to_string(count)) {}
};

struct NotImplemented : public EvalFailure {
    NotImplemented(Gc* gc) : EvalFailure("Not implemented") {}
};

struct ReadError : public EvalFailure {
    ReadError(Gc* gc, const std::string& error_message, long int character)
        : EvalFailure("Read error: " + error_message + ", character: " + std::to_string(character)) {}
};

struct Car {
    EvalResult operator()(void* param, Gc* gc, Scope* scope, const Expr& args);
};

struct Eval {
    EvalResult operator()(Gc* gc, Scope* scope, const Expr& expr);
};

struct EvalBlock {
    EvalResult operator()(Gc* gc, Scope* scope, const Expr& block);
};

struct MatchList {
    EvalResult operator()(Gc* gc, const std::string& format, const Expr& args, ...);
};

#endif  // INTERPRETER_H_
