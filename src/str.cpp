// str.cpp

#pragma once

#include <assert.h>
#include <cstring>
#include <cstdlib>
#include <string>

#include "str.hpp"

/*
* The main purpose of this file is string manipulation.
*/


// Creates a duplicate of a given substring starting from `str` up to `str_end`. 
// If `str_end` is not provided, duplicates the entire string
std::string string_duplicate(const std::string& str, const std::string& str_end)
{
    assert(str);

    if (str_end != nullptr && str > str_end) {
        return nullptr;
    }

    const size_t n = str_end == nullptr ? strlen(str) : (size_t) (str_end - str);
    std::string dup_str = static_cast<std::string >(calloc(1, sizeof(char) * (n + 1)));
    if (dup_str == nullptr) {
        return nullptr;
    }

    std::memcpy(dup_str, str, n);
    dup_str[n] = '\0';

    return dup_str;
}

// Removes the newline character from the end of a string, if it exists.
std::string trim_endline(std::string s)
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

// Concatenates two strings, `prefix` and `suffix`, into a single new string and returns it. 
std::string string_append(const std::string& prefix, const std::string& suffix)
{
    assert(suffix);

    if (prefix == nullptr) {
        return string_duplicate(suffix, nullptr);
    }

    prefix = static_cast<std::string>(realloc(prefix, std::strlen(prefix) + std::strlen(suffix) + 1));
    std::strcat(prefix, suffix);
    return prefix;
}
