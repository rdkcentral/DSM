
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

#ifndef __PACKAGER_ADAPTER_HPP
#define __PACKAGER_ADAPTER_HPP

#include <string>

#include "../../ext/json.hpp"

struct PackageData;

class PackagerAdapter {
   nlohmann::json config;

  public:
   const std::string name{"local-tarball"};
   
   PackagerAdapter();
   void configure(nlohmann::json config);
   auto install(std::shared_ptr<PackageData> package,std::string id, std::string uri) -> nlohmann::json;
   auto uninstall(std::string id) -> nlohmann::json;
   auto is_installed(std::string id) -> bool;
   auto list() -> nlohmann::json;
   auto check_executable(std::string id) -> nlohmann::json;
   private:
   void fork_exe(char* path, char *const args[], const std::function<void()>& fn_callback);
   void fork_exe_wget(std::shared_ptr<PackageData> package, std::string id, std::string uri, std::string dest, std::string localUri);
   void wget_callback_success(std::shared_ptr<PackageData> package, std::string id, std::string uri, std::string dest, std::string localUri);
};

#endif
