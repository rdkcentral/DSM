
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
