
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

int main() {
   std::cout << "Server Started: " << std::endl;
   SocketServer server{"/tmp/test_socket"};
   // initialize is a separate function to make sure fd are initialized in the moment you chose.
   server.initialize();
   auto response = nlohmann::json::parse("{\"ResponseOK\":\"This is correct sample response\"}");
   while (server.awaits()) {
      auto commands_to_process = server.received_commands();
      for (auto command : commands_to_process) {
         std::cout << "Command received: " << command->received() << std::endl;
         command->respond_with("{\"ResponseOK\":\"This is correct sample response\"}");
         std::cout << "Command send: " << response
                   << "   type match:" << (typeid(response) == typeid(nlohmann::json)) << std::endl;
      }
   }
   return 0;
}
