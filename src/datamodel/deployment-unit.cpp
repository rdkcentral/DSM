
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

#include "deployment-unit.hpp"

#include <iostream>

DeploymentUnit::DeploymentUnit(ExecutionEnvironment *parent_ee, std::string uri, std::shared_ptr<Packager> packager)
    : ee(parent_ee), 
      packager(packager),
      uid(uri),
      state(Packager::Undefined),      
      eu(nullptr),
      eu_path("") {
   std::cout << "<<create>> DeploymentUnit (" << uri << ")" << std::endl;
   config["URI"] = uri;
   config["UUID"] = uid;
   packager->append_package(uid, uri);
   packager->on_update(uri,
                       [=](std::string uid, Packager::PackageState new_state) { on_package_update(uid, new_state); });
}
DeploymentUnit::DeploymentUnit(ExecutionEnvironment *parent_ee, const PackageData &installed_package,
                               std::shared_ptr<Packager> packager)
    : ee(parent_ee),
      packager(packager), 
      uid(installed_package.ext_id), 
      state(Packager::Installed),
      eu(nullptr),
      eu_path("") {
   config["URI"] = installed_package.uri;
   config["UUID"] = installed_package.ext_id;
   packager->on_update(installed_package.uri,
                       [=](std::string uid, Packager::PackageState new_state) { on_package_update(uid, new_state); });
}

DeploymentUnit::DeploymentUnit(ExecutionEnvironment *parent_ee, const DeploymentUnit &other)
    : ee(parent_ee), 
      packager(other.packager), 
      uid(other.uid), 
      state(other.state),
      eu(other.eu),
      eu_path(other.eu_path) {      
   std::cout << "<<copy>> DeploymentUnit (" << uid << ")" << std::endl;
   config = other.config;
   packager->on_update(config["URI"],
                       [=](std::string uid, Packager::PackageState new_state) { on_package_update(uid, new_state); });
}

void DeploymentUnit::on_package_update(std::string updated_uid, Packager::PackageState new_state) {
   std::cout << "DeploymentUnit.on_package_update[" << get_uri() << "](status change: " 
             << Packager::state_to_string(state) << " -> " << Packager::state_to_string(new_state)
             << ")" << std::endl;
   state = new_state;
   
   auto info = packager->get_info(updated_uid);

   // Once installed, and no eu yet, and is executable, set up the EU
   if (info["exec"] && eu == nullptr && new_state == Packager::PackageState::Installed)
   {
      eu_path = info["path"];
      eu = std::make_shared<ExecutionUnit>(ee, this);
   }
}

auto DeploymentUnit::parent_ee() -> ExecutionEnvironment * { return ee; }

auto DeploymentUnit::get_uri() const -> std::string { return config["URI"]; };

auto DeploymentUnit::get_state() -> Packager::PackageState { return state; }

auto DeploymentUnit::get_duid() -> std::string { return uid; }

void DeploymentUnit::install() {
   std::cout << "DeploymentUnit.install(" << get_uri() << ")" << std::endl;
   packager->install(uid);
}

void DeploymentUnit::uninstall() {
   std::cout << "DeploymentUnit.uninstall(" << get_uri() << ")" << std::endl;
   std::cout <<"      has_eu:"<<bool(eu)<<std::endl;

   if (eu){
      auto eu_state = eu->get_state();
      if (eu_state == ContainerRuntime::Undefined || eu_state == ContainerRuntime::Idle ){
         eu.reset();
         packager->uninstall(uid);
      }else{
         std::cout << "      error: cannot uninstall because EU is not Undefined or Idle"<<std::endl;
      }

   }else{ // no EU
      packager->uninstall(uid);
   }
}

auto DeploymentUnit::to_json() -> nlohmann::json { return config; }

auto DeploymentUnit::get_detail() -> nlohmann::json {
   auto detail = config;
   detail["parent-ee"] = ee->name();
   detail["state"] = Packager::state_to_string(state);
   detail["eu"] = false;

   if (has_eu()){
      detail["eu"] = eu->get_uid();
      detail["eu.path"] = eu_path;
   }   
   return detail; 
}
auto DeploymentUnit::has_eu() -> bool { return eu != nullptr; }

auto DeploymentUnit::get_eu_path() -> std::string { return eu_path;}

auto DeploymentUnit::get_eu() -> ExecutionUnit*{
   return eu.get();
}
