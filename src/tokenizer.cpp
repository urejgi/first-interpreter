#pragma once

#include <cassert>
#include <cctype>
#include <string>

#include "tokenizer.hpp"

// Creates a Token object representing a portion of a string from 'begin' to 'end'.
Token token(const std::string& begin, const std::string& end)
{
    Token token = {
        .begin = begin.c_str(),
        .end = end.c_str()
    };

    return token;
}

// Determines whether a character is allowed in a symbol, 
// excluding certain forbidden characters and whitespace.
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

// Advances the given iterator to the next character that is not whitespace in the string.
std::string::const_iterator skip_whitespace(std::string::const_iterator begin){
    return std::find_if(begin, str.end(), [](char c) { return !std::isspace(c); });
}

// Identifies the next occurrence of a double quote character (") in the string,
//  used to parse strings in the input.
static const std::string next_quote(const std::string& str)
{
    assert(str);

    while (*str != 0 && *str != '"') {
        str++;
    }

    return str;
}

// Advances through the string until a newline character is encountered, 
// used for handling comments.
static const std::string skip_until_newline(const std::string& str)
{
    assert(str);

    while (*str != 0 && *str != '\n') {
        str++;
    }

    return str;
}

// Finds the next character in the string that does not correspond to a symbol character, 
// aiding in tokenization of the input.
std::string::const_iterator next_non_symbol(std::string::const_iterator begin)
{
    return std::find_if_not(begin, str.end(), is_symbol_char);
}

// Parses the next token from the input string, 
// applying rules for different token types such as symbols, strings, 
// and special characters.
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

