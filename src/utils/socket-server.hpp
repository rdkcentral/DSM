#ifndef __socket_server_hpp
#define __socket_server_hpp

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "command-context.hpp"

class SocketServer {
   std::string fname;
   std::atomic_bool enabled;
   std::unordered_map<int, CommandContext> all_commands;
   std::vector<CommandContext *> commands_await_response;

   int timeout;
   int fd_socket;
   int fd_epoll;
   struct sockaddr_un socket_address;
   struct epoll_event event_accept;

   void init_epoll();
   void init_socket();
   void handle_active_descriptor(int fd, unsigned int events);

  public:
   SocketServer(std::string fname, int timeout = 5000);
   SocketServer(SocketServer &) = delete;
   ~SocketServer();
   void initialize();
   auto awaits() -> bool;
   void disable();
   auto received_commands() -> std::vector<CommandContext *> &;
};

#endif
