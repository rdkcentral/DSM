
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

#include "command-context.hpp"

#include <iostream>

CommandContext::CommandContext() {
   // std::cout << "CommandContext()" << std::endl;
}

CommandContext::~CommandContext() {
   //    std::cout << "~CommandContext()" << std::endl;

   current_state = Destroying;
   cancel_epoll();
}

auto CommandContext::received() -> nlohmann::json {
   assert(current_state != Uninitialized);
   return rcv_command;
}

void CommandContext::assign_connection_fd(int related_fd, int epoll_fd) {
   //    std::cout << "CommandContext::assign_connection_fd(related_fd:" << related_fd << ", epoll_fd:" <<
   //    epoll_fd
   //              << ")" << std::endl;
   this->fd_assigned = related_fd;
   this->fd_epoll = epoll_fd;
   current_state = Receiving;
   event.events = EPOLLIN;  // reading mode
   event.data.fd = this->fd_assigned;
   auto result = epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_assigned, &event);
   if (result < 0) {
      perror("Assigning filedescriptor to epoll");
      return;
   }
}

auto CommandContext::read_fragment() -> int {
   assert(current_state == Receiving);
   if (command_size == UNKNOWN) {
      // std::cout << "  - Header" << std::endl;
      recv(fd_assigned, &command_size, sizeof(command_size), 0);
      incoming_buffer.resize(command_size + 1, 0);  // 0 to make sure string will be null terminated
      return sizeof(command_size);
   }
   auto read_bytes = recv(fd_assigned, &incoming_buffer[0] + total_bytes_read, command_size - total_bytes_read, 0);
   total_bytes_read += read_bytes;
   if (total_bytes_read >= command_size) {
      rcv_command = nlohmann::json::parse(&incoming_buffer[0]);
      // std::cout << "  Finished reading: " << rcv_command << std::endl;
      current_state = AwaitResponse;
      auto result = epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_assigned, &event);
      assert(result >= 0);
   }
   return total_bytes_read;
}

auto CommandContext::send_fragment() -> int {
   assert(current_state == Responding);
   // std::cout << "CommandContext::send_fragment(" << response << ")" << std::endl;

   if (response_size == UNKNOWN) {
      response_size = response.size();
      send(fd_assigned, &response_size, sizeof(response_size), 0);
      return sizeof(response_size);
   }

   auto sent_bytes = send(fd_assigned, response.c_str() + total_bytes_sent, response_size - total_bytes_sent, 0);
   total_bytes_sent += sent_bytes;
   if (total_bytes_sent >= response_size) {  // finished all bytes are sent
      current_state = Done;
      cancel_epoll();
   }
   return total_bytes_sent;
}

void CommandContext::cancel_epoll() {
   if (fd_assigned == UNKNOWN) {
      return;
   }
   auto result = epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_assigned, &event);
   if (result != 0 && current_state != Destroying) {
      perror("Closing epoll subscription failed.");
      assert(result == 0);
   }

   close(fd_assigned);
   fd_assigned = UNKNOWN;
}

auto CommandContext::is_finished() -> bool {
   assert(current_state != Uninitialized);
   return current_state == Done;
}

void CommandContext::respond_with(nlohmann::json result) {
   assert(current_state != Uninitialized);
   // std::cout << "CommandContext::respond_with(" << result << ")" << std::endl;
   response = result.dump();
   current_state = Responding;
   event.events = EPOLLOUT;  // sending mode
   if (0 > epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_assigned, &event)) {
      perror("Assigning filedescriptor to epoll");
      return;
   }
}

auto CommandContext::process() -> bool {
   assert(current_state != Uninitialized);
   //    std::cout << "CommandContext::process()" << std::endl;
   switch (current_state) {
      case Receiving:
         read_fragment();
         return true;
      case Responding:
         send_fragment();
         return true;
      default:
         assert(false);  // any other state is not allowed during epoll socket events
         return false;
   }
}
