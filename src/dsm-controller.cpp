
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

#include "dsm-controller.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

bool has_keys(const nlohmann::json& object, std::vector<std::string> params) {
   for (auto key : params) {
      if (object.find(key) == object.end()) {
         return false;
      }
   }
   return true;
}

DSMController::DSMController() {
   std::cout << "<<create>> DSMConroler" << std::endl;
   build_repository();
}
DSMController::~DSMController() {
   command_server->disable();
   std::cout << "<<delete>> DSMConroler" << std::endl;
}

void DSMController::update_config() {
   config["ExecutionEnvironments"] = nlohmann::json::parse("[]");
   for (auto ee : ee_list_vector) {
      config["ExecutionEnvironments"].push_back(ee->to_json());
   }
}
auto DSMController::save_current_state() -> bool {
   std::string config_file_name{config["DSMController"]["RuntimeConfig"]};
   std::cout << "DSMController::save_current_state(" << config_file_name << ")" << std::endl;
   update_config();
   std::ofstream file(config_file_name, std::ofstream::trunc);
   if (file.fail()) {
      std::cout << "Error: DSMController::save_current_state() can not save data to file: " << config_file_name
                << std::endl;
      return false;
   }
   file << config;
   return true;
}
auto DSMController::load_state() -> bool {
   std::string config_file_name{config["DSMController"]["RuntimeConfig"]};
   std::ifstream file(config_file_name);
   if (file.fail()) {
      std::cout << "  DSMConroler.load_state(" << config_file_name << ") file not found." << std::endl;
      return false;
   }
   file >> config;
   return true;
}
auto DSMController::load_default(std::string config_file_name) -> bool {
   std::ifstream file(config_file_name);
   if (file.fail()) {
      std::cout << "  Error 'no such file' : DSMConroler.load_default(" << config_file_name
                << ") = " << config["DSMController"] << std::endl;
      return false;
   }
   file >> config;
   std::cout << "  DSMConroler.load_default(" << config_file_name << ") = " << config["DSMController"] << std::endl;
   return true;
}

void DSMController::configure(std::string config_file_name) {
   // packagers
   load_default(config_file_name);
   load_state();
   packager = std::make_shared<Packager>(config["Packagers"]);
   runtime = std::make_shared<ContainerRuntime>();
   command_server = std::make_shared<SocketServer>(config["CommandServer"]["domain-socket"]);
   //    command_thread = std::make_shared<std::thread>([=]() { this->command_handler_loop(); });

   // execution environments
   // config = local_config["DSMController"];
   for (auto ee_config : config["ExecutionEnvironments"]) {
      ee_list_vector.push_back(std::make_shared<ExecutionEnvironment>(ee_config, packager, runtime));
   }

   // update packages according to latest configuration. If package not added yet, add it to the deafult ee
   auto default_ee = default_execution_environment();
   if (default_ee != nullptr) {
      auto packages = packager->list();
      std::cout << "Default:" << default_ee->name() << std::endl;
      for (auto package : packages) {
         bool package_in_ee{false};
         std::cout << "Package:" << package->ext_id << std::endl;
         for (auto ee : ee_list_vector) {
            if (ee->has_du_in_config(package->ext_id)) {
               std::cout << "+EE: " << ee->name() << std::endl;
               package_in_ee = true;
               auto du = ee->add_existing(*package);
               du_list_vector.push_back(du);
               break;
            }
         }
         if (!package_in_ee) {
            std::cout << "+Default" << std::endl;
            auto du = default_ee->add_existing(*package);
            du_list_vector.push_back(du);
         }
      }
   }
}

auto DSMController::find_execution_environment(unsigned int id) -> std::shared_ptr<ExecutionEnvironment> {
   auto elem = std::find_if(begin(ee_list_vector), end(ee_list_vector),
                            [=](std::shared_ptr<ExecutionEnvironment> ee_ptr) { return id == ee_ptr->id(); });
   if (elem == end(ee_list_vector)) return nullptr;
   return *elem;
}


auto DSMController::find_deployment_unit(std::string uri) -> std::shared_ptr<DeploymentUnit> 
{
   std::lock_guard<std::mutex> lock (du_list_mutex);
   auto du =
       find_if(du_list_vector.begin(), du_list_vector.end(), [=](std::shared_ptr<DeploymentUnit> du) { return uri == du->get_duid(); });
   if (du == du_list_vector.end()) {
      return nullptr;
   }
   return (*du);
}

auto DSMController::find_execution_environment(const std::string& ee_name) -> std::shared_ptr<ExecutionEnvironment> {
   auto elem = std::find_if(begin(ee_list_vector), end(ee_list_vector),
                            [=](std::shared_ptr<ExecutionEnvironment> ee_ptr) { return ee_name == ee_ptr->name(); });
   if (elem == end(ee_list_vector)) return nullptr;
   return *elem;
}

auto DSMController::default_execution_environment() -> std::shared_ptr<ExecutionEnvironment> {
   auto elem = std::find_if(begin(ee_list_vector), end(ee_list_vector),
                            [=](std::shared_ptr<ExecutionEnvironment> ee_ptr) { return ee_ptr->is_default(); });
   if (elem == end(ee_list_vector)) return nullptr;
   return *elem;
}

void DSMController::command_handler_loop() {
   command_server->initialize();
   while (command_server->awaits()) {
      // std::cout << "Awaits -> awake" << std::endl;
      for (auto command : command_server->received_commands()) {
         std::cout << "CDSMController::command_handler_loop() COMMAND:" << std::endl;
         auto response = command_dispatcher(command->received());
         std::cout << "   in:" << command->received() << std::endl;
         std::cout << "  out:" << response << std::endl;
         command->respond_with(response);
      }
   }
}

auto DSMController::command_dispatcher(const nlohmann::json& command) -> nlohmann::json {
   nlohmann::json ret;
   //    ret["method"] = command["method"];
   std::string method(command["method"]);
   ret["id"] = command["id"];
   if (command_repository.find(method) == command_repository.end()) {
      ret["error"] = std::string("Method '") + std::string(command["method"]) + std::string("' is not implemented");
      return ret;
   }
   nlohmann::json params = command.find("params") == command.end() ? nlohmann::json::parse("{}") : command["params"];
   std::cout << "dsm." << method << "(" << params << ")" << std::endl;
   auto command_result = command_repository[method](params);
   if (has_keys(command_result, {"error"})) {
      ret["error"] = command_result["error"];
      return ret;
   }
   ret["result"] = command_result;

   return ret;
}

void DSMController::build_repository() {
   command_repository["ee.list"] = [=](const nlohmann::json& input) { return ee_list(input); };
   command_repository["ee.detail"] = [=](const nlohmann::json& input) { return ee_detail(input); };

   command_repository["du.list"] = [=](const nlohmann::json& input) { return du_list(input); };
   command_repository["du.install"] = [=](const nlohmann::json& input) { return du_install(input); };
   command_repository["du.uninstall"] = [=](const nlohmann::json& input) { return du_uninstall(input); };
   command_repository["du.update"] = [=](const nlohmann::json& input) { return du_update(input); };
   command_repository["du.list"] = [=](const nlohmann::json& input) { return du_list(input); };
   command_repository["du.detail"] = [=](const nlohmann::json& input) { return du_detail(input); };
   command_repository["du.reassign"] = [=](const nlohmann::json& input) { return du_reassign(input); };

   command_repository["eu.list"] = [=](const nlohmann::json& input) { return eu_list(input); };
   command_repository["eu.detail"] = [=](const nlohmann::json& input) { return eu_detail(input); };
   command_repository["eu.start"] = [=](const nlohmann::json& input) { return eu_start(input); };
   command_repository["eu.stop"] = [=](const nlohmann::json& input) { return eu_stop(input); };
   command_repository["dsm.save_state"] = [=](const nlohmann::json& input) { return dsm_save_state(input); };
}

auto DSMController::du_install(const nlohmann::json params) -> nlohmann::json {
   std::cout << "DSMController::install_du()" << std::endl;
   auto output = nlohmann::json::parse("{}");
   try
   {
      if (!has_keys(params, {"ee_name", "uri"})) {
         return nlohmann::json::parse(R"( {"error":"install_du: missing expected parameters 'ee_name','uri'."} )");
      }
      std::string ee_name = params["ee_name"];
      std::string uri = params["uri"];
      auto ee = find_execution_environment(ee_name);

      if (nullptr == ee) {
         return nlohmann::json::parse(R"( {"error":"install_du: target environment doesn't exists"} )");
      }

      auto du = ee->install(uri);
      if (du == nullptr){
         output["response"] = "Package already installed";
         return output;
      }
      {
         std::lock_guard<std::mutex> lock (du_list_mutex);
         du_list_vector.push_back(du);
      }
      std::cout << "DSMController::du_list += " << du->get_uri() <<"  ["<<du->get_detail()["state"] <<"]"<< std::endl;
      auto output = nlohmann::json::parse("{}");
      output["response"] = "Installation Started";
      return output;
   }
   catch (nlohmann::json::parse_error& e)
    {
        // output exception information
        std::cout << "message: " << e.what() << '\n'
                  << "exception id: " << e.id << '\n'
                  << "byte position of error: " << e.byte << std::endl;
      auto output = nlohmann::json::parse("{}");
      output["response"] = "Installation Failed";
      return output;
    }
}

auto DSMController::du_uninstall(const nlohmann::json params) -> nlohmann::json {
   auto duid_to_unistall = params["uuid"];
   std::cout << "DSMController::uninstall_du(uuid:" << duid_to_unistall << ")" << std::endl;
   auto du =
       find_if(du_list_vector.begin(), du_list_vector.end(), [=](std::shared_ptr<DeploymentUnit> du) 
       { 
         return duid_to_unistall == du->get_duid(); 
       });
   if (du == du_list_vector.end()) {
      return nlohmann::json::parse(R"( {"error":"uninstall_du: du not found."} )");
   }
   bool ret = (*du)->uninstall();
   if (ret)
   {
      std::lock_guard<std::mutex> lock (du_list_mutex);
      du_list_vector.erase(du);
      return nlohmann::json::parse(R"("Uninstallation started")");
   }
   else
   {
      return nlohmann::json::parse(R"( {"error":"uninstall_du: Failed"} )");  
   }
}
auto DSMController::du_update(const nlohmann::json params) -> nlohmann::json { return "not implemented"; }
auto DSMController::du_reassign(const nlohmann::json params) -> nlohmann::json { return "not implemented"; }

auto DSMController::du_list(const nlohmann::json params) -> nlohmann::json {
   auto ret = nlohmann::json::parse("[]");
   std::lock_guard<std::mutex> lock (du_list_mutex);
   for (auto du : du_list_vector) {
      ret += du->get_uri();
   }
   return ret;
}

auto DSMController::du_detail(const nlohmann::json params) -> nlohmann::json { 
   auto duid = params["uuid"];
   std::cout << "DSMController::detail_du(uuid:" << duid << ")" << std::endl;
   std::lock_guard<std::mutex> lock (du_list_mutex);
   auto du =
       find_if(du_list_vector.begin(), du_list_vector.end(), [=](std::shared_ptr<DeploymentUnit> du) { return duid == du->get_duid(); });
   if (du == du_list_vector.end()) {
      return nlohmann::json::parse(R"( {"error":"detail_du: du not found."} )");
   }
   return (*du)->get_detail();
}

auto DSMController::ee_list(const nlohmann::json params) -> nlohmann::json {
   auto ret = nlohmann::json::parse("[]");
   for (auto ee : ee_list_vector) {
      ret += ee->name();
   }
   return ret;
}

auto DSMController::eu_list(const nlohmann::json params) -> nlohmann::json { 
   auto ret = nlohmann::json::parse("[]");
   std::lock_guard<std::mutex> lock (eu_list_mutex);
   for (auto ee: ee_list_vector){
      for (auto eu : ee->get_eu_list()) {
         ret += eu->get_uid();
      }
   }
   return ret;   
};

auto DSMController::find_execution_unit(const std::string &uid) -> ExecutionUnit *{
   // TODO: very similar to eu_list - refactor it to use vector of eus
   std::lock_guard<std::mutex> lock (eu_list_mutex);
   for (auto ee : ee_list_vector){
      for (auto eu: ee->get_eu_list()){
         auto get_uid = eu->get_uid();
         if (uid == eu->get_uid()){
            return eu;
         }
      }
   }
   return nullptr;
}

auto DSMController::eu_detail(const nlohmann::json params) -> nlohmann::json { 
   auto str_params = params.dump();
   auto eu_uid = params["uid"];
   auto str_eu_uid = eu_uid.dump();
   auto eu = find_execution_unit(eu_uid);
   if (eu == nullptr){
      return nlohmann::json::parse(R"( {"error":"detail_eu: eu not found."} )");
   }
   return eu->get_detail();
};

auto DSMController::eu_start(const nlohmann::json params) -> nlohmann::json { 
   auto eu_uid = params["uid"];
   auto eu = find_execution_unit(eu_uid);
   if(eu == nullptr){
      return nlohmann::json::parse(R"( {"error":"detail_eu: eu not found."} )");
   }
   std::cout << "   DSMController::eu_start("<< eu_uid<<", "<< eu->get_state()<<")"<<std::endl;

   if (eu->get_state() != ContainerRuntime::Idle){
      return nlohmann::json::parse(R"( {"error":"Cannot start EU which is not Idle."} )");    
   }
   std::cout << "       + STARTING"<<std::endl;
   eu->start();
   std::cout << "       - STARTING"<<std::endl;

   return nlohmann::json::parse(R"("Starting EU")");
};
auto DSMController::eu_stop(const nlohmann::json params) -> nlohmann::json { 
   auto eu_uid = params["uid"];
   auto eu = find_execution_unit(eu_uid);
   if(eu == nullptr){
      return nlohmann::json::parse(R"( {"error":"detail_eu: eu not found."} )");
   }
   
   std::cout << "   DSMController::eu_stop("<< eu_uid<<", "<< eu->get_state()<<")"<<std::endl;

   if (eu->get_state() != ContainerRuntime::Active){
      return nlohmann::json::parse(R"( {"error":"Cannot stop EU which is not Active."} )");
   }
   eu->stop();
   return nlohmann::json::parse(R"("Stopping EU")");
};

auto DSMController::ee_detail(const nlohmann::json params) -> nlohmann::json { return "not implemented"; };
auto DSMController::dsm_save_state(const nlohmann::json params) -> nlohmann::json {
   if (save_current_state()) {
      return config["DSMController"]["RuntimeConfig"];
   }
   return nlohmann::json::parse(R"( {"error":"install_du: missing expected parameters 'ee_name','uri'."} )");
};
