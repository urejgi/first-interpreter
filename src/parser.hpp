#ifndef PARSER_H_
#define PARSER_H_

#pragma once

#include <fstream>
#include <string>
#include <system_error>
#include "expr.hpp"
#include "tokenizer.hpp"

struct ParseError : public std::runtime_error {
    explicit ParseError(const std::string& what) : std::runtime_error(what) {}
};

struct ParseResult
{
    bool is_error;
    const char* end;
    union {
        Expr expr;
        const char* error_message;
    };

    ParseResult(bool is_error_, const char* end_, const Expr& expr_)
        : is_error(is_error_), end(end_), expr(expr_) {}

    ParseResult(bool is_error_, const char* end_, const std::string& error_message_)
        : is_error(is_error_), end(end_), error_message(error_message_) {}
};

ParseResult parse_success(const Expr& expr, const char* end);
ParseResult parse_failure(const char* error, const char* end);

ParseResult read_expr_from_string(Gc* gc, const std::string& str);
ParseResult read_all_exprs_from_string(Gc* gc, const std::string& str);

ParseResult read_expr_from_file(Gc* gc, const std::string& filename);
ParseResult read_all_exprs_from_file(Gc* gc, const std::string& filename);

void print_parse_error(std::ostream& stream,
    const std::string& str,
    const ParseResult& result);

#endif  // PARSER_H_