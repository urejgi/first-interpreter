#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <string>

struct Token
{
    const char* begin;
    const char* end;
};

Token token(const std::string& begin, const std::string& end)
Token next_token(const std::string& str);
static bool is_symbol_char(char x);
std::string::const_iterator skip_whitespace(std::string::const_iterator begin);
static const std::string next_quote(const std::string& str)
static const std::string skip_until_newline(const std::string& str);
std::string::const_iterator next_non_symbol(std::string::const_iterator begin);

#endif  // TOKENIZER_H_
