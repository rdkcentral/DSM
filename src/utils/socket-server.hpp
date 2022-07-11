
// If not stated otherwise in this file or this component's license file the
// following copyright and licenses apply:
//
// Copyright 2022 Consult Red
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
