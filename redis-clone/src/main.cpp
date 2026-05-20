#include "storage.hpp"
#include "eviction.hpp"
#include "parser.hpp"
#include "network.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <cctype>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 6379
#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

int main() {
    int server_fd = setupServerPort(PORT);
    if (server_fd == -1) {
        return 1;
    }

    int kq = kqueue();
    if (kq == -1) {
        std::cerr << "kqueue failed" << std::endl;
        return 1;
    }

    struct kevent change_event;
    EV_SET(&change_event, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1) {
        std::cerr << "kevent failed" << std::endl;
        return 1;
    }

    ThreadSafeDB db;
    TTLEngine ttl_engine(db);
    std::unordered_map<int, ClientState> clients;
    struct kevent event_list[MAX_EVENTS];

    std::cout << "In-Memory RESP Database listening on port " << PORT << std::endl;

    while (true) {
        int nev = kevent(kq, NULL, 0, event_list, MAX_EVENTS, NULL);
        if (nev == -1) {
            if (errno == EINTR) continue;
            std::cerr << "kevent wait failed" << std::endl;
            break;
        }

        for (int i = 0; i < nev; i++) {
            int fd = event_list[i].ident;

            if (event_list[i].flags & EV_EOF) {
                close(fd);
                clients.erase(fd);
            } 
            else if (fd == server_fd) {
                while (true) {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlen = sizeof(client_address);
                    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen);
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; 
                        }
                        std::cerr << "Accept failed" << std::endl;
                        break;
                    }
                    setNonBlocking(client_fd);
                    
                    int client_opt = 1;
                    setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &client_opt, sizeof(client_opt));

                    EV_SET(&change_event, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
                    kevent(kq, &change_event, 1, NULL, 0, NULL);
                    clients[client_fd] = ClientState();
                }
            } 
            else if (event_list[i].filter == EVFILT_READ) {
                char buffer[BUFFER_SIZE];
                ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
                
                if (bytes_read == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        close(fd);
                        clients.erase(fd);
                    }
                } else if (bytes_read == 0) {
                    close(fd);
                    clients.erase(fd);
                } else {
                    auto& state = clients[fd];
                    state.read_buffer.append(buffer, bytes_read);
                    
                    while (true) {
                        std::vector<std::string> tokens;
                        int consumed = RESPParser::parse(state.read_buffer, tokens);
                        
                        if (consumed == 0) {
                            break; // Incomplete
                        } else if (consumed == -1) {
                            state.write_buffer += "-ERR protocol error\r\n";
                            state.read_buffer.clear();
                            break;
                        }
                        
                        if (!tokens.empty()) {
                            std::string cmd = tokens[0];
                            for (auto& c : cmd) c = toupper(c);

                            std::string response;
                            if (cmd == "PING") {
                                response = "+PONG\r\n";
                            } else if (cmd == "SET" && tokens.size() >= 3) {
                                std::string key = tokens[1];
                                std::string value = tokens[2];
                                
                                if (tokens.size() >= 5) {
                                    std::string opt = tokens[3];
                                    for (auto& c : opt) c = toupper(c);
                                    if (opt == "EX") {
                                        int ex = 0;
                                        for (char c : tokens[4]) {
                                            if (isdigit(c)) ex = ex * 10 + (c - '0');
                                        }
                                        db.set(key, value);
                                        ttl_engine.addTTL(key, ex);
                                        response = "+OK\r\n";
                                    } else {
                                        response = "-ERR syntax error\r\n";
                                    }
                                } else {
                                    db.set(key, value);
                                    response = "+OK\r\n";
                                }
                            } else if (cmd == "GET" && tokens.size() >= 2) {
                                std::string key = tokens[1];
                                std::string value;
                                if (db.get(key, value)) {
                                    response = "$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
                                } else {
                                    response = "$-1\r\n";
                                }
                            } else {
                                response = "-ERR unknown command\r\n";
                            }
                            
                            state.write_buffer += response;
                        }
                        
                        state.read_buffer.erase(0, consumed);
                    }
                    
                    if (!state.write_buffer.empty()) {
                        EV_SET(&change_event, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
                        kevent(kq, &change_event, 1, NULL, 0, NULL);
                    }
                }
            } 
            else if (event_list[i].filter == EVFILT_WRITE) {
                auto it = clients.find(fd);
                if (it != clients.end()) {
                    auto& state = it->second;
                    if (!state.write_buffer.empty()) {
                        ssize_t sent = write(fd, state.write_buffer.c_str(), state.write_buffer.length());
                        if (sent > 0) {
                            state.write_buffer.erase(0, sent);
                        } else if (sent == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
                            close(fd);
                            clients.erase(fd);
                            continue;
                        }
                    }
                    
                    if (state.write_buffer.empty()) {
                        EV_SET(&change_event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
                        kevent(kq, &change_event, 1, NULL, 0, NULL);
                    }
                }
            }
        }
    }
    
    close(server_fd);
    return 0;
}
