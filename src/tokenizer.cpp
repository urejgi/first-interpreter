#pragma once

#include <cassert>
#include <cctype>
#include <string>

#include "tokenizer.hpp"

// Converts a string range into a Token object.
Token token(const std::string& begin, const std::string& end)
{
    Token token = {
        .begin = begin.c_str(),
        .end = end.c_str()
    };

    return token;
}

// Checks if a character is a forbidden symbol character.
static bool is_symbol_char(char x)
{
    static constexpr char forbidden_symbol_chars[] = {
        '(', ')', '"', '\'', ';', '`', ','
    };
    static constexpr size_t n = sizeof(forbidden_symbol_chars) / sizeof(char);

    for (size_t i = 0; i < n; ++i) {
        if (x == forbidden_symbol_chars[i] || std::isspace(x)) {
            return false;
        }
    }

    return true;
}

// Advances the iterator to the next non-whitespace character.
std::string::const_iterator skip_whitespace(std::string::const_iterator begin){
    return std::find_if(begin, str.end(), [](char c) { return !std::isspace(c); });
}

// Finds the next quote character in a string.
static const std::string next_quote(const std::string& str)
{
    assert(str);

    while (*str != 0 && *str != '"') {
        str++;
    }

    return str;
}

// Finds the next newline character in a string.
static const std::string skip_until_newline(const std::string& str)
{
    assert(str);

    while (*str != 0 && *str != '\n') {
        str++;
    }

    return str;
}

// Finds the next non-symbol character in a string.
std::string::const_iterator next_non_symbol(std::string::const_iterator begin)
{
    return std::find_if_not(begin, str.end(), is_symbol_char);
}

// Returns the next token in the input string.
Token next_token(const std::string& str)
{
    assert(str);

    str = skip_whitespace(str);
    if (*str == 0) {
        return token(str, str);
    }

    while (*str != 0 && *str == ';') {
        str = skip_until_newline(str + 1);
        str = skip_whitespace(str);
    }

    switch (*str) {
    case '(':
    case ')':
    case '.':
    case '\'':
    case '`':
    case ',':
        return token(str, str + 1);

    case '"': {
        const std::string str_end = next_quote(str + 1);
        return token(str, *str_end == 0 ? str_end : str_end + 1);
    }

    default:
        return token(str, next_non_symbol(str + 1));
    }
}

