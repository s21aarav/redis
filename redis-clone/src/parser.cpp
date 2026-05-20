#include "parser.hpp"
#include <cctype>

int RESPParser::parse(const std::string& buffer, std::vector<std::string>& tokens) {
    tokens.clear();
    size_t pos = 0;
    
    if (buffer.empty()) return 0; 
    if (buffer[0] != '*') return -1; // Must be an array
    
    pos++; 
    int num_args = 0;
    while (pos < buffer.size() && buffer[pos] != '\r') {
        if (!isdigit(buffer[pos])) return -1;
        num_args = num_args * 10 + (buffer[pos] - '0');
        pos++;
    }
    
    if (pos + 1 >= buffer.size()) return 0; 
    if (buffer[pos] != '\r' || buffer[pos+1] != '\n') return -1; 
    pos += 2; 
    
    for (int i = 0; i < num_args; ++i) {
        if (pos >= buffer.size()) return 0; 
        if (buffer[pos] != '$') return -1; // Must be a bulk string
        pos++; 
        
        int len = 0;
        while (pos < buffer.size() && buffer[pos] != '\r') {
            if (!isdigit(buffer[pos])) return -1;
            len = len * 10 + (buffer[pos] - '0');
            pos++;
        }
        if (pos + 1 >= buffer.size()) return 0; 
        if (buffer[pos] != '\r' || buffer[pos+1] != '\n') return -1; 
        pos += 2; 
        
        if (pos + len > buffer.size()) return 0; 
        tokens.push_back(buffer.substr(pos, len));
        pos += len;
        
        if (pos + 1 >= buffer.size()) return 0; 
        if (buffer[pos] != '\r' || buffer[pos+1] != '\n') return -1; 
        pos += 2; 
    }
    return pos; 
}
