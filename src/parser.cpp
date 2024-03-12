// parser.cpp

/*
* This code file contains an implementation of a parser for a Lisp-like language. 

    It reads a string or a file and converts it into an abstact syntax tree (AST).

    The parser is implemented in C++ 
        and uses a hand-written recursive descent parsing approach.
*/


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

/*
* Parses the "cdr" (tail) part of a Lisp pair when a dot notation is encountered.
* Expected syntax: `. expression )` where `expression` is any valid Lisp expression representing the cdr.
* It confirms the presence of a '.' followed by a valid expression and a closing parenthesis.
* Returns a `ParseResult` indicating success along with the parsed expression, or an error if the syntax is incorrect.
*/
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

/*
* Handles the end of a list parsing when a closing parenthesis ')' is found.
* Constructs and returns a `ParseResult` representing the end of a list, typically denoted by 'nil' in Lisp.
* This function ensures proper list termination in accordance with Lisp syntax rules.
*/
static ParseResult parse_list_end(Gc *gc, Token current_token)
{
    if (current_token.begin[0] != ')') {
        return parse_failure("Expected )", current_token.begin);
    }

    return parse_success(atom_as_expr(create_symbol_atom(gc, "nil", nullptr)),
                         current_token.end);
}

/*
* Parses a sequence of Lisp expressions enclosed in parentheses, treating it as a list.
* Recursively constructs the internal structure of the list by creating cons cells for each expression parsed,
  until it encounters either a dot (for cdr notation) or the closing parenthesis.
* Deals with proper list structures (`(a b c)`) as well as dotted pairs (`(a . b)`).
* Returns a `ParseResult` containing the fully constructed list or an error in case of syntax violations.
*/
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


/*
* Parses a string literal enclosed in double quotes, including support for escaped characters.
* Escaped characters (e.g., `\"`, `\\`, `\n`, and others) are interpreted according to their standard meanings.
* The function builds the string character by character, 
    handling escapes appropriately until the closing quote is reached.
* Returns a `ParseResult` with the parsed string or an error
    if the string is malformed (e.g., unclosed, invalid escape sequences).
*/
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

/*
* Parses numeric expressions expected to represent integers in Lisp expressions.
* Utilizes `std::strtol` to convert the token string to a long integer.
* Verifies that the entire token is a valid integer, not just a substring, ensuring accurate parsing.
* Returns a `ParseResult` containing the parsed integer as an atom 
    or an error if the token cannot be parsed as an integer.
*/
static ParseResult parse_integer(Gc *gc, Token current_token)
{
    std::string endptr = nullptr;
    const long int x = std::strtol(current_token.begin, &endptr, 10);

    if ((current_token.begin == endptr) || (current_token.end != endptr)) {
        return parse_failure("Expected integer", current_token.begin);
    }

    return parse_success(
        atom_as_expr(create_integer_atom(gc, x)),
        current_token.end);
}

/*
* Parses numeric expressions expected to represent real numbers (floating-point values) in Lisp expressions.
* Uses `std::strtof` to attempt conversion of the token string to a float.
* Ensures the entire token represents the floating-point number to prevent partial matches and potential parsing inaccuracies.
* Generates a `ParseResult` with the parsed real number as an atom or an error for invalid inputs.
*/
static ParseResult parse_real(Gc *gc, Token current_token)
{
    assert(gc);

    std::string endptr = nullptr;
    const float x = std::strtof(current_token.begin, &endptr);

    if ((current_token.begin == endptr) || (current_token.end != endptr)) {
        return parse_failure("Expected real", current_token.begin);
    }

    return parse_success(REAL(gc, x), current_token.end);
}

/*
* Handles parsing of symbols in Lisp expressions.
* Symbols are non-numeric, non-string expressions 
    that correspond to variable names or special identifiers.
* Converts the token directly into a symbol atom without modification, preserving case and format.
* Produces a `ParseResult` featuring the created symbol atom 
    or an error if it encounters an EOF character unexpectedly.
*/
static ParseResult parse_symbol(Gc *gc, Token current_token)
{
    if (current_token.begin[0] == 0) {
        return parse_failure("EOF", current_token.begin);
    }

    return parse_success(
        atom_as_expr(create_symbol_atom(gc, current_token.begin, current_token.end)),
        current_token.end);
}

/*
* The central function for parsing any kind of expression in a Lisp-like language.

* Distinguishes between different types of expressions 
    based on the initial character(s) of the token (e.g., '(', '"', or numerical).

* Recursively processes expressions, supporting nested structures and special forms
    (quote, quasiquote, unquote).

* Returns a `ParseResult` containing the parsed expression or an error, 
    effectively acting as a dispatcher to more specific parsing functions.
*/
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

/*
* Initiates the parsing process for a single expression from a string.

* This function is designed to convert a raw string into a Lisp expression using `parseexpr.

* Primarily used for handling simple inputs or evaluating expressions from user input 
    or files at a single-expression granularity.

* Ensures basic preconditions (non-emptiness of the string) before dispatching to the parsing logic.
*/
ParseResult read_expr_from_string(Gc *gc, const std::string& str)
{
    assert(gc);
    assert(!str.empty());
    return parse_expr(gc, next_token(str));
}

/*
* Designed for parsing multiple Lisp expressions from a single string.
* Iteratively calls `parse_expr` to sequentially parse expressions, building a list of these expressions.
* Accommodates for multiple expressions separated by standard Lisp delimiters, enabling batch processing of Lisp code.
* Returns a `ParseResult` containing a list of all parsed expressions or an error if any part of the string violates Lisp syntax.
*/
ParseResult read_all_exprs_from_string(Gc *gc, const std::string& str)
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


/*
* Utility function for generating a parsing error `ParseResult` based on I/O or system errors.
* Utilizes the errno value to retrieve a human-readable error message using `std::strerror`.
* Intended for use in situations where an I/O operation fails, offering a mechanism to signal I/O related errors during parsing.
*/
ParseResult parse_io_failure(int errno)
{
    ParseResult result = {
        .is_error = true,
        .error_message = std::strerror(errno),
        .end = nullptr
    };

    return result;
}


/*
* Reads and parses a single expression from a file.
* The function asserts that the provided filename is non-empty.
* Attempts to open the specified file in binary mode and returns an error if it fails, utilizing `parse_io_failure`.
* Checks if the file is too big or empty and ensures it can access both the beginning and the end of the file.
* Reads the entire file content into a string buffer and then parses a single expression from this buffer using `read_expr_from_string`.
* Returns a `ParseResult` structure that either contains the parsed expression or an error message.
*/
ParseResult read_expr_from_file(Gc *gc, const std::string& filename)
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


/*
* Reads and parses all expressions from a file into a list.
* Opens the file in binary mode and handles errors similarly to `read_expr_from_file`.
* Validates the file's accessibility, checks if the file is too big, empty, or if it can locate the file's start and end.
* Loads the file content into a string buffer and uses `read_all_exprs_from_string` to parse all expressions present.
* Returns a `ParseResult` which either contains a list of all parsed expressions or an error message.
*/
ParseResult read_all_exprs_from_file(Gc *gc, const std::string& filename)
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


/*
* Constructs a success `ParseResult`.

* Initializes a `ParseResult` object with the successful parsing result, 
    the parsed expression, and the location in the input string where parsing ended.

* Indicates the lack of errors by setting `is_error` to false.

* This function is used to encapsulate a successful parsing outcome, 
    providing a uniform interface for handling parse results.
*/
ParseResult parse_success(Expr expr, const std::string& end)
{
    ParseResult result = {
        .is_error = false,
        .expr = expr,
        .end = end
    };

    return result;
}


/*
* Constructs a failure `ParseResult`.

* Initializes a `ParseResult` object with the provided error message 
    and the location in the input where the error was detected.

* Indicates the presence of an error by setting `is_error` to true.

* Allows for a consistent method of signaling parsing errors across the parsing subsystem.
*/
ParseResult parse_failure(const std::string& error_message, const std::string& end)
{
    ParseResult result = {
        .is_error = true,
        .error_message = error_message,
        .end = end
    };

    return result;
}

/*
* Removes leading and trailing whitespace from a given string.

* Utilizes `std::string::find_first_not_of` and `std::string::find_last_not_of` 
    to identify the bounds of the trimmed string.

* Returns a substring of the original string that excludes any leading 
    or trailing whitespace characters.

* This utility function aids in string preprocessing for error display 
    and other needs where whitespace may skew the interpretation.
*/
static std::string trim_whitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

/*
* Outputs a formatted error message for a parsing failure to a given file stream.

* Checks if there is actually an error to report; exits early if not.

* Computes the line and column number where the error occurred by iterating over the characters up to the error point.

* Then, it prints the error location (line and column), and the error message to the provided `FILE*` stream.

* It intelligently trims the output to relevant portions around where the error occurred for better readability.

* Designed to provide detailed feedback on parsing errors, facilitating debugging and error correction.
*/

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


