#pragma once
#include <string>
#include <vector>

class RESPParser {
public:
    static int parse(const std::string& buffer, std::vector<std::string>& tokens);
};
