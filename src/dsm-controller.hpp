
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

#ifndef __DSMCONTROLLER_HPP
#define __DSMCONTROLLER_HPP

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ext/json.hpp"
#include "utils/socket-server.hpp"
#include "datamodel/execution-environment.hpp"
#include "datamodel/bridge/packager.hpp"
#include "datamodel/bridge/container-runtime.hpp"

class DSMController {
   nlohmann::json config;
   std::vector<std::shared_ptr<ExecutionEnvironment> > ee_list_vector;
   std::vector<std::shared_ptr<DeploymentUnit>> du_list_vector;
   std::mutex du_list_mutex;
   std::mutex eu_list_mutex;
   
   
   std::shared_ptr<Packager> packager{nullptr};
   std::shared_ptr<ContainerRuntime> runtime{nullptr};
   std::shared_ptr<SocketServer> command_server{nullptr};
   std::unordered_map<std::string, std::function<nlohmann::json(const nlohmann::json&)> > command_repository;

   // std::shared_ptr<std::thread> command_thread{nullptr}; // try to do it in main loop
   void build_repository();
   void update_config();
   auto save_current_state() -> bool;
   auto load_state() -> bool;
   auto load_default(std::string config_file_name) -> bool;

  public:
   DSMController();
   ~DSMController();
   void command_handler_loop();
   void configure(std::string config_file_name);
   
   auto find_deployment_unit(std::string uid) -> std::shared_ptr<DeploymentUnit>;

   auto find_execution_environment(unsigned int i) -> std::shared_ptr<ExecutionEnvironment>;
   auto find_execution_environment(const std::string& name) -> std::shared_ptr<ExecutionEnvironment>;
   auto find_execution_unit(const std::string &uid) -> ExecutionUnit *;
   auto default_execution_environment() -> std::shared_ptr<ExecutionEnvironment>;
   auto command_dispatcher(const nlohmann::json& command) -> nlohmann::json;

   // methods install called remotely
   auto du_install(const nlohmann::json params) -> nlohmann::json;
   auto du_uninstall(const nlohmann::json params) -> nlohmann::json;
   auto du_update(const nlohmann::json params) -> nlohmann::json;
   auto du_reassign(const nlohmann::json params) -> nlohmann::json;
   auto du_list(const nlohmann::json params) -> nlohmann::json;
   auto du_detail(const nlohmann::json params) -> nlohmann::json;

   auto ee_list(const nlohmann::json params) -> nlohmann::json;
   auto ee_detail(const nlohmann::json params) -> nlohmann::json;

   auto eu_list(const nlohmann::json params) -> nlohmann::json;
   auto eu_detail(const nlohmann::json params) -> nlohmann::json;
   auto eu_start(const nlohmann::json params) -> nlohmann::json;
   auto eu_stop(const nlohmann::json params) -> nlohmann::json;
   
   auto dsm_save_state(const nlohmann::json params) -> nlohmann::json;
};

#endif
