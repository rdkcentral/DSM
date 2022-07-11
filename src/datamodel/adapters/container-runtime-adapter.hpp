
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

#ifndef __CONTAINER_RUNTIME_ADAPTER_HPP
#define __CONTAINER_RUNTIME_ADAPTER_HPP

#include <string>

#include "../../ext/json.hpp"

class ContainerRuntimeAdapter {
   nlohmann::json config;

  public:
  #ifdef CONTAINER_RUNTIME_USE_CRUN
   const std::string name{"crun"};
  #else
   const std::string name{"dobby"};
  #endif

   ContainerRuntimeAdapter();
   void configure(nlohmann::json config);
   auto start(std::string id, std::string uri) -> nlohmann::json;
   auto stop(std::string id) -> nlohmann::json;
   auto detail(std::string id) -> nlohmann::json;
   auto list() -> nlohmann::json;  

   bool use_dobby; // True by default, use crun if false
};

#endif