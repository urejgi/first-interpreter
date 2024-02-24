#ifndef STR_H_
#define STR_H_

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#include <string>

std::string string_duplicate(const std::string& str);
std::string string_(const std::string& prefix, const std::string& suffix);
std::string trim_endline(std::string str);

#endif  // STR_H_
