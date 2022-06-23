
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

#include "execution-unit.hpp"
#include "../utils/uuid_generator.hpp"
#include <iostream>

ExecutionUnit::ExecutionUnit(ExecutionEnvironment *parent_ee, DeploymentUnit *parent_du)
        :ee(parent_ee),
         du(parent_du),
         uid(generate_UUID(2)),
         state(ContainerRuntime::Idle)
        {
    std::cout<< "<<create>> ExecutionUnit ["<< uid <<"] EE:"<< parent_ee->name() <<"  DU:"<< parent_du->get_duid() <<std::endl;
    std::cout<< "           Path: ["<< du->get_eu_path()<<"]" <<std::endl;
    
}

void ExecutionUnit::start(){
    std::cout<<"["<<uid<<"].ExecutionUnit::start(path:"<<du->get_eu_path()<<")" << std::endl;
    ee->get_runtime()->start(uid, du->get_eu_path());
}

void ExecutionUnit::stop(){
    std::cout<<"["<<uid<<"].ExecutionUnit::stop(path:"<<du->get_eu_path()<<")" << std::endl;
    ee->get_runtime()->stop(uid);
}

auto ExecutionUnit::get_state() -> ContainerRuntime::ContainerState {
    // std::cout<< "ExecutionUnit::get_state() uid="<<uid<<std::endl;
    ContainerRuntime *runtime = ee->get_runtime();
    auto state =  runtime->getInfo(uid)["status"];
    if (state == "Idle") return ContainerRuntime::Idle;
    if (state == "Active") return ContainerRuntime::Active;
    if (state == "Starting") return ContainerRuntime::Starting;
    if (state == "Stopping") return ContainerRuntime::Stopping;
    return ContainerRuntime::Undefined;
}

auto ExecutionUnit::get_detail() -> nlohmann::json {
    ContainerRuntime *runtime = ee->get_runtime();
    nlohmann::json ret_detail = runtime->getInfo(uid);

    ret_detail["uid"] = uid;
    ret_detail["ee"]= ee->name();
    ret_detail["du"]= du->get_duid();    
    ret_detail["path"]= du->get_eu_path();

    return ret_detail;
}
auto ExecutionUnit::get_uid() -> std::string{
    return uid;
}
