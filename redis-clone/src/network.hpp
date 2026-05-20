#pragma once
#include <string>

struct ClientState {
    std::string read_buffer;
    std::string write_buffer;
};

void setNonBlocking(int fd);
int setupServerPort(int port);
