#ifndef __COMMAND_CONTEXT_HPP
#define __COMMAND_CONTEXT_HPP

#define UNKNOWN -1
#define MAX_PATH_SIZE 108

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../ext/json.hpp"

class SocketServer;

class CommandContext {
   nlohmann::json rcv_command;
   std::string response;

   int command_size{UNKNOWN};
   int response_size{UNKNOWN};
   int total_bytes_read{0};
   int total_bytes_sent{0};
   std::vector<char> incoming_buffer;

   int fd_assigned{UNKNOWN};
   int fd_epoll{UNKNOWN};
   struct epoll_event event;

   auto read_fragment() -> int;
   auto send_fragment() -> int;
   void cancel_epoll();

   friend class SocketServer;
   enum State { Uninitialized, Receiving, AwaitResponse, Responding, Done, Destroying };
   State current_state{Receiving};
   void assign_connection_fd(int related_fd, int fd_epoll);
   auto is_finished() -> bool;
   auto process() -> bool;

  public:
   CommandContext();
   ~CommandContext();
   auto received() -> nlohmann::json;
   void respond_with(nlohmann::json result);
};

#endif
