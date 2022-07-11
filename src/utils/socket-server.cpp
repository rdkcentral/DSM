
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

#include "socket-server.hpp"

#include <iostream>

///////////////////////////////     INTERNAL HELP  ///////////////////////////////////////////////////////////
auto events_to_str(unsigned int event) -> std::string {
   std::string output;

   if (event & EPOLLIN) output += " EPOLLIN";
   if (event & EPOLLOUT) output += " EPOLLOUT";
   if (event & EPOLLRDHUP) output += " EPOLLRDHUP";
   if (event & EPOLLPRI) output += " EPOLLPRI";
   if (event & EPOLLERR) output += " EPOLLERR";
   if (event & EPOLLHUP) output += " EPOLLHUP";
   if (event & EPOLLET) output += " EPOLLET";
   if (event & EPOLLONESHOT) output += " EPOLLONESHOT";
   if (event & EPOLLWAKEUP) output += " EPOLLWAKEUP";
   if (event & EPOLLEXCLUSIVE) output += " EPOLLEXCLUSIVE";
   return output;
}

///////////////////////////////  SOCKET SERVER ///////////////////////////////////////////////////////////////
#define MAX_PATH_SIZE 108
#define LISTEN_QUEUE_SIZE 5
SocketServer::SocketServer(std::string fname, int timeout) : fname(fname), enabled(true), timeout(timeout) {
   // std::cout << "++create++ SocketServer >> fname: " << fname << std::endl;
}

SocketServer::~SocketServer() {
   close(fd_socket);
   close(fd_epoll);
   unlink(fname.c_str());
   // std::cout << "--delete-- SocketServer >> fname: " << fname << std::endl;
}

void SocketServer::initialize() {
   init_socket();
   init_epoll();
}

void SocketServer::init_epoll() {
   // std::cout << "SocketServer::init_epoll()" << std::endl;
   if (-1 == (fd_epoll = epoll_create1(0))) {
      perror("SocketServer::init_epoll() - Initialization of epoll:");
      return;
   }
   // std::cout << "Initialized epoll: " << fd_epoll << std::endl;
   event_accept.events = EPOLLIN;  // reading mode
   event_accept.data.fd = fd_socket;
   epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_socket, &event_accept);
}

void SocketServer::init_socket() {
   // std::cout << "SocketServer::init_socket()" << std::endl;
   unlink(fname.c_str());
   fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);
   if (-1 == fd_socket) {
      perror("SocketServer::init_socket() - Creating Socket: ");
      return;
   }
   socket_address.sun_family = AF_UNIX;
   strncpy(socket_address.sun_path, fname.c_str(), MAX_PATH_SIZE);
   if (0 > bind(fd_socket, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_un))) {
      perror("SocketServer::init_socket() - Binding domain scoket to the file: ");
      enabled = false;
      return;
   }
   if (0 > listen(fd_socket, LISTEN_QUEUE_SIZE)) {
      perror("SocketServer::init_socket() - Listening on the socket: ");
      enabled = false;
      return;
   }
}

void SocketServer::handle_active_descriptor(int fd_ready, unsigned int events) {
   // std::cout << "handle_active_descriptor(" << fd_ready << ", " << events_to_str(events) << ")"
   //           << std::endl;
   if (fd_ready == fd_socket) {  // new connection
      auto fd_new = accept(fd_socket, 0, 0);
      if (fd_new < 0) {
         perror("SocketServer::handle_active_descriptor() - Accepting new connection: ");
         return;
      }
      all_commands[fd_new].assign_connection_fd(fd_new, fd_epoll);
   } else {
      all_commands[fd_ready].process();
   }
}

void SocketServer::disable() { enabled = false; }

auto SocketServer::awaits() -> bool {
   // std::cout << "SocketServer::awaits()" << std::endl;

   struct epoll_event events[LISTEN_QUEUE_SIZE];

   int events_count = epoll_wait(fd_epoll, events, LISTEN_QUEUE_SIZE, timeout);
   if (events_count < 0) {
      if (errno != EINTR) {
         perror("SocketServer::awaits() - epoll_wait: ");
         return false;
      } else {
         return enabled;
      }
   }

   // std::cout << "Awaits received events: " << events_count << std::endl;
   for (int i = 0; i < events_count && enabled; i++) {
      bool remove_connection = false;
      auto fd = events[i].data.fd;

      // std::cout << "Event on fd:" << fd << " " << events_to_str(events[i].events) << std::endl;
      if (events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP) {
         remove_connection = true;
      }
      handle_active_descriptor(fd, events[i].events);
      if (all_commands.find(fd) != all_commands.end()) {  // check it exists - avoid creation of a new command
         if (all_commands[fd].current_state == CommandContext::Done) {
            remove_connection = true;
         }
      }
      if (remove_connection) {
         // std::cout << "        awaits() - removing command (" << fd << ")" << std::endl;
         all_commands.erase(fd);
      }
   }
   return enabled;
}

auto SocketServer::received_commands() -> std::vector<CommandContext*>& {
   // std::cout << "SocketServer::received_commands()" << std::endl;
   commands_await_response.clear();
   for (auto& fd_command_pair : all_commands) {
      auto command = &fd_command_pair.second;
      if (command->current_state == CommandContext::AwaitResponse) {
         // std::cout << "received_commnads() " << command << " -> " << command->received() << std::endl;
         // std::cout << "      " << command << " -> " << command->received() << std::endl;
         commands_await_response.push_back(command);
      }
   }
   return commands_await_response;
}
