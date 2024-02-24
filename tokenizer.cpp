#include <cassert>
#include <cctype>
#include <string>

#include "tokenizer.hpp"

Token token(const std::string& begin, const std::string& end)
{
    Token token = {
        .begin = begin.c_str(),
        .end = end.c_str()
    };

    return token;
}

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

std::string::const_iterator skip_whitespace(std::string::const_iterator begin){
    return std::find_if(begin, str.end(), [](char c) { return !std::isspace(c); });
}

static const char* next_quote(const char* str)
{
    assert(str);

    while (*str != 0 && *str != '"') {
        str++;
    }

    return str;
}

static const char* skip_until_newline(const char* str)
{
    assert(str);

    while (*str != 0 && *str != '\n') {
        str++;
    }

    return str;
}

std::string::const_iterator next_non_symbol(std::string::const_iterator begin)
{
    return std::find_if_not(begin, str.end(), is_symbol_char);
}


Token next_token(const char* str)
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
        const char* str_end = next_quote(str + 1);
        return token(str, *str_end == 0 ? str_end : str_end + 1);
    }

    default:
        return token(str, next_non_symbol(str + 1));
    }
}
