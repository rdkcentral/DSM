
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

#ifndef __DEPLOYMENTUNIT_HPP
#define __DEPLOYMENTUNIT_HPP

#include <string>

#include "../ext/json.hpp"
#include "execution-environment.hpp"
#include "execution-unit.hpp"
#include "bridge/packager.hpp"

class ExecutionEnvironment;
class ExecutionUnit;

class DeploymentUnit {
   ExecutionEnvironment *ee;
   std::shared_ptr<Packager> packager;
   std::string uid;
   Packager::PackageState state;
   nlohmann::json config;   
   std::shared_ptr<ExecutionUnit> eu;
   std::string eu_path;

   void link_eu_if_exists();

  public:
   DeploymentUnit(ExecutionEnvironment *parent_ee, std::string uri, std::shared_ptr<Packager> packager);
   DeploymentUnit(ExecutionEnvironment *parent_ee, const PackageData &installed_package,
                  std::shared_ptr<Packager> packager);
   DeploymentUnit(ExecutionEnvironment *parent_ee, const DeploymentUnit &other);
   DeploymentUnit &operator=(const DeploymentUnit &) = delete;
   auto get_uri() const -> std::string;
   auto get_state() -> Packager::PackageState;
   auto get_duid() -> std::string;
   void install();
   void uninstall();
   auto parent_ee() -> ExecutionEnvironment *;
   void on_package_update(std::string updated_uri, Packager::PackageState new_state);
   auto to_json() -> nlohmann::json;
   auto get_detail() -> nlohmann::json;
   auto has_eu() -> bool;
   auto get_eu_path() -> std::string;  
   auto get_eu() -> ExecutionUnit*;
   
};
#endif
