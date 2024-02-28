// parser.cpp

#pragma once

#include <assert.h>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "builtins.hpp"
#include "parser.hpp"

#define MAX_BUFFER_LENGTH (5 * 1000 * 1000)

static ParseResult parse_expr(Gc *gc, Token current_token);

static ParseResult parse_cdr(Gc *gc, Token current_token)
{
    if (current_token.begin[0] != '.') {
        return parse_failure("Expected .", current_token.begin);
    }

    ParseResult cdr = read_expr_from_string(gc, std::string_view(current_token.end));
    if (cdr.is_error) {
        return cdr;
    }

    current_token = next_token(cdr.end);

    if (current_token.begin[0] != ')') {
        return parse_failure("Expected )", current_token.begin);
    }

    return parse_success(cdr.expr, current_token.end);
}

static ParseResult parse_list_end(Gc *gc, Token current_token)
{
    if (current_token.begin[0] != ')') {
        return parse_failure("Expected )", current_token.begin);
    }

    return parse_success(atom_as_expr(create_symbol_atom(gc, "nil", nullptr)),
                         current_token.end);
}

static ParseResult parse_list(Gc *gc, Token current_token)
{
    if (current_token.begin[0] != '(') {
        return parse_failure("Expected (", current_token.begin);
    }

    current_token = next_token(current_token.end);

    if (current_token.begin[0] == ')') {
        return parse_list_end(gc, current_token);
    }

    ParseResult car = parse_expr(gc, current_token);
    if (car.is_error) {
        return car;
    }

    Cons *list = create_cons(gc, car.expr, void_expr());
    Cons *cons = list;
    current_token = next_token(car.end);

    while (current_token.begin[0] != '.' &&
           current_token.begin[0] != ')' &&
           current_token.begin[0] != 0) {
        car = parse_expr(gc, current_token);
        if (car.is_error) {
            return car;
        }

        cons->cdr = cons_as_expr(create_cons(gc, car.expr, void_expr()));
        cons = cons->cdr.cons;

        current_token = next_token(car.end);
    }

    ParseResult cdr = (current_token.begin[0] == '.')
        ? parse_cdr(gc, current_token)
        : parse_list_end(gc, current_token);

    if (cdr.is_error) {
        return cdr;
    }

    cons->cdr = cdr.expr;

    return parse_success(cons_as_expr(list), cdr.end);
}


static ParseResult parse_string(Gc* gc, Token current_token)
{
    if (current_token.begin[0] != '"') {
        return parse_failure("Expected \"", current_token.begin);
    }

    std::string str;
    bool escaped = false;

    for (const char* c = current_token.begin + 1; *c != 0; ++c) {
        if (escaped) {
            switch (*c) {
            case 'n':
                str += '\n';
                break;
            case 'r':
                str += '\r';
                break;
            case 't':
                str += '\t';
                break;
            case '\\':
                str += '\\';
                break;
            case '\"':
                str += '"';
                break;
            default:
                return parse_failure("Invalid escaped character", c);
            }
            escaped = false;
        }
        else if (*c == '\\') {
            escaped = true;
        }
        else if (*c == '"') {
            return parse_success(
                atom_as_expr(create_string_atom(gc, str.c_str(), nullptr)),
                c + 1);
        }
        else {
            str += *c;
        }
    }

    return parse_failure("Unclosed string", current_token.begin);
}


static ParseResult parse_integer(Gc *gc, Token current_token)
{
    char *endptr = nullptr;
    const long int x = std::strtol(current_token.begin, &endptr, 10);

    if ((current_token.begin == endptr) || (current_token.end != endptr)) {
        return parse_failure("Expected integer", current_token.begin);
    }

    return parse_success(
        atom_as_expr(create_integer_atom(gc, x)),
        current_token.end);
}

static ParseResult parse_real(Gc *gc, Token current_token)
{
    assert(gc);

    char *endptr = nullptr;
    const float x = std::strtof(current_token.begin, &endptr);

    if ((current_token.begin == endptr) || (current_token.end != endptr)) {
        return parse_failure("Expected real", current_token.begin);
    }

    return parse_success(REAL(gc, x), current_token.end);
}

static ParseResult parse_symbol(Gc *gc, Token current_token)
{
    if (current_token.begin[0] == 0) {
        return parse_failure("EOF", current_token.begin);
    }

    return parse_success(
        atom_as_expr(create_symbol_atom(gc, current_token.begin, current_token.end)),
        current_token.end);
}

static ParseResult parse_expr(Gc *gc, Token current_token)
{
    if (current_token.begin[0] == 0) {
        return parse_failure("EOF", current_token.begin);
    }

    switch (current_token.begin[0]) {
    case '(': return parse_list(gc, current_token);
    case '"': return parse_string(gc, current_token);
    case '\'': {
        ParseResult result = parse_expr(gc, next_token(current_token.end));

        if (result.is_error) {
            return result;
        }

        result.expr = list(gc, "qe", "quote", result.expr);

        return result;
    } break;

    case '`': {
        ParseResult result = parse_expr(gc, next_token(current_token.end));

        if (result.is_error) {
            return result;
        }

        result.expr = list(gc, "qe", "quasiquote", result.expr);

        return result;
    } break;

    case ',': {
        ParseResult result = parse_expr(gc, next_token(current_token.end));

        if (result.is_error) {
            return result;
        }

        result.expr = list(gc, "qe", "unquote", result.expr);

        return result;
    } break;

    default: {}
    }

    if (current_token.begin[0] == '-' || std::isdigit(current_token.begin[0])) {
        ParseResult result;

        result = parse_integer(gc, current_token); if (!result.is_error) return result;
        result = parse_real(gc, current_token); if (!result.is_error) return result;
    }

    return parse_symbol(gc, current_token);
}


ParseResult read_expr_from_string(Gc *gc, const std::string &str)
{
    assert(gc);
    assert(!str.empty());
    return parse_expr(gc, next_token(str));
}

ParseResult read_all_exprs_from_string(Gc *gc, const std::string &str)
{
    assert(gc);
    assert(!str.empty());

    Token current_token = next_token(str);
    if (*(current_token.end) == 0) {
        return parse_success(NIL(gc), current_token.end);
    }

    ParseResult parse_result = parse_expr(gc, current_token);
    if (parse_result.is_error) {
        return parse_result;
    }

    Cons *head = create_cons(gc, parse_result.expr, void_expr());
    Cons *cons = head;

    current_token = next_token(parse_result.end);
    while (*(current_token.end) != 0) {
        parse_result = parse_expr(gc, current_token.begin);
        if (parse_result.is_error) {
            return parse_result;
        }

        cons->cdr = CONS(gc, parse_result.expr, void_expr());
        cons = cons->cdr.cons;
        current_token = next_token(parse_result.end);
    }

    cons->cdr = NIL(gc);

    return parse_success(cons_as_expr(head), parse_result.end);
}

ParseResult parse_io_failure(int errno)
{
    ParseResult result = {
        .is_error = true,
        .error_message = std::strerror(errno),
        .end = nullptr
    };

    return result;
}

ParseResult read_expr_from_file(Gc *gc, const std::string &filename)
{
    assert(filename.size() > 0);

    std::ifstream stream(filename, std::ios::binary);
    if (!stream) {
        return parse_io_failure(errno);
    }

    if (stream.seekg(0, std::ios::end) != std::ios::end) {
        return parse_failure("Could not find the end of the file", nullptr);
    }

    const long int buffer_length = stream.tellg();

    if (buffer_length < 0) {
        return parse_failure("Couldn't get the size of file", nullptr);
    }

    if (buffer_length == 0) {
        return parse_failure("File is empty", nullptr);
    }

    if (buffer_length >= MAX_BUFFER_LENGTH) {
        return parse_failure("File is too big", nullptr);
    }

    if (stream.seekg(0, std::ios::beg) != std::ios::beg) {
        return parse_failure("Could not find the beginning of the file", nullptr);
    }

    std::string buffer(buffer_length, ' ');
    stream.read(&buffer[0], buffer_length);

    ParseResult result = read_expr_from_string(gc, buffer);

    return result;
}

ParseResult read_all_exprs_from_file(Gc *gc, const std::string &filename)
{
    std::ifstream stream(filename, std::ios::binary);
    if (!stream) {
        return parse_io_failure(errno);
    }

    if (stream.seekg(0, std::ios::end) != std::ios::end) {
        return parse_failure("Could not find the end of the file", nullptr);
    }

    const long int buffer_length = stream.tellg();

    if (buffer_length < 0) {
        return parse_failure("Couldn't get the size of file", nullptr);
    }

    if (buffer_length == 0) {
        return parse_failure("File is empty", nullptr);
    }

    if (buffer_length >= MAX_BUFFER_LENGTH) {
        return parse_failure("File is too big", nullptr);
    }

    if (stream.seekg(0, std::ios::beg) != std::ios::beg) {
        return parse_failure("Could not find the beginning of the file", nullptr);
    }

    std::string buffer(buffer_length, ' ');
    stream.read(&buffer[0], buffer_length);

    ParseResult result = read_all_exprs_from_string(gc, buffer);

    return result;
}

ParseResult parse_success(Expr expr, const char *end)
{
    ParseResult result = {
        .is_error = false,
        .expr = expr,
        .end = end
    };

    return result;
}

ParseResult parse_failure(const std::string &error_message, const char *end)
{
    ParseResult result = {
        .is_error = true,
        .error_message = error_message,
        .end = end
    };

    return result;
}


static std::string trim_whitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}


void print_parse_error(FILE* stream, const std::string& str, ParseResult result)
{
    if (!result.is_error) {
        return;
    }

    size_t line_number = 1;
    size_t column_number = 1;
    size_t current_column = 1;

    for (const char* c = str.c_str(); c < result.end; ++c) {
        if (*c == '\n') {
            line_number++;
            column_number = current_column;
        }
        else {
            current_column++;
        }
    }

    fprintf(stream, "Parse error at line %zu, column %zu:\n", line_number, column_number);

    if (result.end) {
        std::string trimmed_str = trim_whitespace(std::string(str.begin(), result.end));
        size_t line_length = trimmed_str.size();

        for (size_t i = 0; i < line_number - 1; ++i) {
            fprintf(stream, "%s\n", trimmed_str.c_str());
        }

        for (size_t i = 0; i < column_number - 1; ++i) {
            fprintf(stream, " ");
        }

        size_t start = std::max((size_t)0, column_number - 20);
        size_t end = std::min(line_length, column_number + 20);

        for (size_t i = start; i < end; ++i) {
            fprintf(stream, "%c", trimmed_str[i]);
        }

        fprintf(stream, "^\n");
    }

    fprintf(stream, "%s\n", result.error_message.c_str());
}


