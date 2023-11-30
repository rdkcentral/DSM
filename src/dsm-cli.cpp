
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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>   

#include "ext/json.hpp"

#define MAX_PATH_SIZE 108

class SocketClient {
   std::string fname;
   int fd_socket;

   auto send_commnad(const nlohmann::json &command) -> bool {
      std::string str_command{command.dump()};
      int command_size = str_command.size();
      auto ret = send(fd_socket, &command_size, sizeof(command_size), 0);
      if (ret < 0) {
         perror("Sending header");
         return false;
      }
      ret = send(fd_socket, str_command.c_str(), command_size, 0);
      if (ret < 0) {
         perror("Sending command");
         return false;
      }
      return false;
   }

   auto receive_reply() -> nlohmann::json {
      int reply_size;
      auto ret = recv(fd_socket, &reply_size, sizeof(reply_size), 0);
      if (ret != sizeof(reply_size)) {
         perror("Receiving header");
         return "";
      }
      std::vector<char> buffer;
      buffer.resize(reply_size + 1);
      ret = recv(fd_socket, &buffer[0], reply_size, 0);
      if (ret != reply_size) {
         perror("Receiving body");
         return "";
      }
      return nlohmann::json::parse(buffer);
   }

  public:
   SocketClient(std::string fname) : fname(fname) {
      // std::cout << "Scoket: " << fname << std::endl;

      fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);
      if (fd_socket < 0) {
         perror("Opening socket");
         return;
      }
      // std::cout << "  socket descriptor: " << fd_socket << std::endl;

      struct sockaddr_un socket_address;

      socket_address.sun_family = AF_UNIX;
      strncpy(socket_address.sun_path, fname.c_str(), MAX_PATH_SIZE);
      // std::cout << "  path: " << socket_address.sun_path << std::endl;

      if (connect(fd_socket, (struct sockaddr *)&socket_address, sizeof(socket_address))) {
         perror("Connecting to server");
         return;
      }
   }
   ~SocketClient() {
      // std::cout << "~Scoket: " << fname << std::endl;
      close(fd_socket);
   }

   auto execute_command(const nlohmann::json &command) -> nlohmann::json {
      if (fd_socket < 0) {
         std::cout << "Error - no connection socket:" << fd_socket << std::endl;
         return "{\"Error\":\"Connection error\"}";
      }
      send_commnad(command);
      return receive_reply();
   }
};

nlohmann::json cmd_ees_list(int argc, char **argv) {
   // simple return no parameters required
   return nlohmann::json::parse(R"({"method":"ee.list", "id":1})");
}

nlohmann::json cmd_dus_list(int argc, char **argv) {
   // simple return no parameters required
   return nlohmann::json::parse(R"({"method":"du.list", "id":1})");
}
nlohmann::json cmd_du_install(int argc, char **argv) {
   // simple return no parameters required
   if (argc != 2) {
      std::cout << "Wrong number of arguments in 'install' expected 2 (ee_name and uri)." << std::endl;
      exit(1);
   }
   std::string ee(argv[0]);
   std::string uri(argv[1]);

   auto ret = nlohmann::json::parse(R"({"method":"du.install", "id":1})");
   auto params = nlohmann::json::parse("{}");
   params["ee_name"] = ee;
   params["uri"] = uri;
   ret["params"] = params;
   return ret;
}

nlohmann::json cmd_du_uninstall(int argc, char **argv) {
   if (argc != 1) {
      std::cout << "Wrong number of arguments in 'uninstall' expected 1 (uuid)." << std::endl;
      exit(1);
   }
   std::string uuid(argv[0]);

   auto ret = nlohmann::json::parse(R"({"method":"du.uninstall", "id":1})");
   auto params = nlohmann::json::parse("{}");
   params["uuid"] = uuid;
   ret["params"] = params;
   return ret;
}
nlohmann::json cmd_du_update(int argc, char **argv) {
   return nlohmann::json::parse(R"({"method":"du.update", "id":1})");
}
nlohmann::json cmd_du_reassign(int argc, char **argv) {
   return nlohmann::json::parse(R"({"method":"du.reassign", "id":1})");
}
nlohmann::json cmd_ee_detail(int argc, char **argv) {
   return nlohmann::json::parse(R"({"method":"ee.detail", "id":1})");
}
nlohmann::json cmd_eus_list(int argc, char **argv) {
   // simple return no parameters required
   return nlohmann::json::parse(R"({"method":"eu.list", "id":1})");
}
nlohmann::json cmd_eu_detail(int argc, char **argv) {
   if (argc != 1){
      std::cout << "Wrong number of arguments in 'detail_eu' expected 1 (uuid)." << std::endl;
      exit(1);
   }
   std::string uid(argv[0]);

   auto ret = nlohmann::json::parse(R"({"method":"eu.detail", "id":1})");
   auto params = nlohmann::json::parse("{}");
   params["uid"] = uid;
   ret["params"] = params;
   return ret;
}
nlohmann::json cmd_du_detail(int argc, char **argv) {
   if (argc != 1){
      std::cout << "Wrong number of arguments in 'detail_du' expected 1 (uuid)." << std::endl;
      exit(1);
   }
   std::string uuid(argv[0]);

   auto ret = nlohmann::json::parse(R"({"method":"du.detail", "id":1})");
   auto params = nlohmann::json::parse("{}");
   params["uuid"] = uuid;
   ret["params"] = params;
   return ret;

   // return nlohmann::json::parse(R"({"method":"cmd_detail_du", "id":1})");
}
nlohmann::json cmd_eu_start(int argc, char **argv) {
   if (argc != 1){
      std::cout << "Wrong number of arguments in 'eu.start' expected 1 (uuid)." << std::endl;
      exit(1);
   }
   std::string uid(argv[0]);

   auto ret = nlohmann::json::parse(R"({"method":"eu.start", "id":1})");
   auto params = nlohmann::json::parse("{}");
   params["uid"] = uid;
   ret["params"] = params;
   return ret;
}
nlohmann::json cmd_eu_stop(int argc, char **argv) {
   if (argc != 1){
      std::cout << "Wrong number of arguments in 'eu.stop' expected 1 (uuid)." << std::endl;
      exit(1);
   }
   std::string uid(argv[0]);

   auto ret = nlohmann::json::parse(R"({"method":"eu.stop", "id":1})");
   auto params = nlohmann::json::parse("{}");
   params["uid"] = uid;
   ret["params"] = params;
   return ret;
}

nlohmann::json cmd_save_state(int argc, char **argv) {
   if (argc != 0) {
      std::cout << "Wrong number of arguments in 'state' expected 0." << std::endl;
      exit(1);
   }
   return nlohmann::json::parse(R"({"method":"dsm.save_state", "id":1})");
}

nlohmann::json build_command(int argc, char **argv) {
   std::map<std::string, std::function<nlohmann::json(int, char **)> > command_factory;
   command_factory["ee.list"] = cmd_ees_list;
   command_factory["ni.ee.detail"] = cmd_ee_detail;
   command_factory["du.list"] = cmd_dus_list;
   command_factory["du.install"] = cmd_du_install;
   command_factory["du.uninstall"] = cmd_du_uninstall;
   command_factory["ni.du.update"] = cmd_du_update;
   command_factory["ni.du.reassign"] = cmd_du_reassign;
   command_factory["du.detail"] = cmd_eu_detail;
   command_factory["du.detail"] = cmd_du_detail;
   command_factory["eu.list"] = cmd_eus_list;
   command_factory["eu.detail"] = cmd_eu_detail;
   command_factory["eu.start"] = cmd_eu_start;
   command_factory["eu.stop"] = cmd_eu_stop;
   command_factory["dsm.save"] = cmd_save_state;

   if (argc <= 0) {
      std::cout << "Wrong number of arguments" << std::endl;
      std::cout << "Available commands are:" << std::endl;
      for (auto kv : command_factory) {
         std::cout << "  " << kv.first << std::endl;
      }
      exit(1);
   }

   if (command_factory.find(argv[0]) == command_factory.end()) {
      std::cout << "Unknown command: " << argv[0] << std::endl;
      std::cout << "Available commands are:" << std::endl;
      for (auto kv : command_factory) {
         std::cout << "  " << kv.first << std::endl;
      }
      exit(1);
   }
   return command_factory[argv[0]](argc - 1, argv + 1);
}

auto get_command_file()
{
   const char *configFile;
   configFile = std::getenv("DSM_CONFIG_FILE");
   if (configFile == nullptr)
   {
      configFile = "../../src/dsm.config";
   }
   std::ifstream file(configFile);
   nlohmann::json config;
   std::string command_socket_filename = "";
   if (file.fail()) {
     std::cout << "  Error 'no such file' : DSMCLI load_config" << configFile
                << std::endl;
   }
   else {

      file >> config;
      command_socket_filename = config["CommandServer"]["domain-socket"];
      std::cout << "  DSMCLI Loaded command.socket file from : " << command_socket_filename << std::endl;
   }
   return command_socket_filename;
}


int main(int argc, char **argv) {
   auto command = build_command(argc - 1, argv + 1);
   // SocketClient socket{"./command.socket"};
   SocketClient socket{get_command_file()};

   std::cout << "Command: " << command << std::endl;
   auto response = socket.execute_command(command);
   std::cout << "Response: " << response << std::endl;
}
