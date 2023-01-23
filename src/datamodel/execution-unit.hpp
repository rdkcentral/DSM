
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

#ifndef __EXECUTION_UNIT_HPP
#define __EXECUTION_UNIT_HPP

#include "../ext/json.hpp"
#include "../utils/uuid_generator.hpp"
#include "execution-environment.hpp"
#include "deployment-unit.hpp"
#include "bridge/container-runtime.hpp"

class ExecutionEnvironment;
class DeploymentUnit;

class ExecutionUnit{
    ExecutionEnvironment *ee;
    DeploymentUnit *du;    
    std::string uid;
    ContainerRuntime::ContainerState state;

    // nlohmann::json config;

    public:
        ExecutionUnit(ExecutionEnvironment *parent_ee, DeploymentUnit *parent_du);
        ~ExecutionUnit();
        ExecutionUnit &operator= (const ExecutionUnit &) = delete;

        void start();
        void stop();
        auto get_state() -> ContainerRuntime::ContainerState;
        auto get_detail() -> nlohmann::json;
        auto get_uid() -> std::string;
};

#endif

