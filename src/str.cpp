// str.cpp

#pragma once

#include <assert.h>
#include <cstring>
#include <cstdlib>
#include <string>

#include "str.hpp"

char *string_duplicate(const char *str, const char *str_end)
{
    assert(str);

    if (str_end != nullptr && str > str_end) {
        return nullptr;
    }

    const size_t n = str_end == nullptr ? strlen(str) : (size_t) (str_end - str);
    char *dup_str = static_cast<char *>(calloc(1, sizeof(char) * (n + 1)));
    if (dup_str == nullptr) {
        return nullptr;
    }

    std::memcpy(dup_str, str, n);
    dup_str[n] = '\0';

    return dup_str;
}

char *trim_endline(char *s)
{
    const size_t n = std::strlen(s);

    if (n == 0) {
        return s;
    }

    if (s[n - 1] == '\n') {
        s[n - 1] = '\0';
    }

    return s;
}

char *string_append(char *prefix, const char *suffix)
{
    assert(suffix);

    if (prefix == nullptr) {
        return string_duplicate(suffix, nullptr);
    }

    prefix = static_cast<char *>(realloc(prefix, std::strlen(prefix) + std::strlen(suffix) + 1));
    std::strcat(prefix, suffix);
    return prefix;
}


