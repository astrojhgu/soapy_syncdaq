#ifndef UTILS_HPP
#define UTILS_HPP
#include <cstdint>
#include <string>
#include <vector>

uint32_t parse_ipv4(const char* input);
std::string format_ipv4(uint32_t ip);
std::vector<int> parse_int_list(const std::string& s);

#endif
