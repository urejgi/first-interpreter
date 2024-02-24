#ifndef TOKENIZER_H_
#define TOKENIZER_H_
#include <string>
struct Token
{
    const char* begin;
    const char* end;
};

Token next_token(const std::string& str);

#endif  // TOKENIZER_H_
