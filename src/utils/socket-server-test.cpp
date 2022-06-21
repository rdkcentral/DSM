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
