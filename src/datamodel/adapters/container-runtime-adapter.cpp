
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

#include "container-runtime-adapter.hpp"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../../utils/execute-command.hpp"

ContainerRuntimeAdapter::ContainerRuntimeAdapter()
{
#ifdef CONTAINER_RUNTIME_USE_CRUN
    use_dobby = false;
#else
    use_dobby = true;
#endif
}

void ContainerRuntimeAdapter::configure(nlohmann::json config){
    std::cout<< "ContainerRuntimeAdapter::configure(nlohmann::json config)" << config <<std::endl;    
}
auto ContainerRuntimeAdapter::start(std::string id, std::string path) -> nlohmann::json{
    std::cout<< "ContainerRuntimeAdapter::start(std::string id, std::string uri) -> nlohmann::json" << id << "," << path<<std::endl;
    nlohmann::json ret;
    
    // this is only the minimum brute force implementation for CLI

    if (use_dobby) // Use Dobby
    {
        std::string command_start = "DobbyTool start " + id + " " + path;

        std::system(command_start.c_str());
    }
    else // Use Crun
    {
        std::string command_delete = "crun delete " + id ;
        std::string command_create = "cd "+path+" && crun create " + id ;
        std::string command_start = "crun start " + id;
        std::string command_state = "crun state " + id;

        std::cout<<"ContainerRuntimeAdapter::start() -> \n   <<delete>> " << 
                command_delete <<"\n   <<create>>  "<< 
                command_create <<"\n   <<start>>  "<< 
                command_start  <<"\n   <<state>>  "<< 
                command_state  <<std::endl;
        
        std::system(command_delete.c_str());
        std::system(command_create.c_str());
        std::system(command_start.c_str());
        std::system(command_state.c_str());
    }

    return ret;
}
auto ContainerRuntimeAdapter::stop(std::string id) -> nlohmann::json{
    std::cout<< "ContainerRuntimeAdapter::stop(std::string id) -> nlohmann::json" << id<<std::endl;
    nlohmann::json ret;

    if (use_dobby) // Use Dobby
    {
        std::string command_stop = "DobbyTool stop " + id;
        std::string result;

        int ret_code;

        std::tie(ret_code, result) = execute_command(command_stop);
    }
    else // Use Crun
    {
        std::string command_kill = "crun kill "+id+" KILL";
        int ret_code;
        std::string result;
        std::tie(ret_code, result) = execute_command(command_kill);

        std::cout<<"ContainerRuntimeAdapter::stop() ->" 
                << "\n    command: "<<command_kill
                << "\n    ret_code:"<< ret_code
                << "\n    result:"<< result
                <<std::endl;
    }
    
    return ret;
}

auto ContainerRuntimeAdapter::detail(std::string id) -> nlohmann::json{
    std::cout<< "ContainerRuntimeAdapter::detail(std::string id) -> nlohmann::json" << id<<std::endl;
    nlohmann::json ret;

    std::string result;
    int ret_code;

    if (use_dobby) // Use Dobby
    {
        std::string command_state = "DobbyTool info " + id;    
        std::tie(ret_code, result) = execute_command(command_state);
    }
    else // Use Crun
    {
        std::string command_state = "crun state "+id;    
        std::tie(ret_code, result) = execute_command(command_state);
    }

    nlohmann::json ret_detail {};

    // Check for no JSON (Dobby does this with no active container)
    if (result.substr(0,5) == "error")
    {
        ret_detail["status"] = "Idle";
        // If no container, dobby returns an error AND no JSON
        return ret_detail; // Assume idle
    }

    if (ret_code == 0){
        // Translate data from execution env to TR-Datamodel
        auto cmd_result = nlohmann::json::parse(result);

        // ret_detail["state"] = cmd_result["status"];
        // Test for JSON format of both crun & dobby (Why are these the same!?)
        if (     cmd_result["status"] == "running" || cmd_result["state"] == "running")  ret_detail["status"] = "Active";
        else if (cmd_result["status"] == "stopped" || cmd_result["state"] == "stopped")  ret_detail["status"] = "Idle";
        else if (cmd_result["status"] == "exited"  || cmd_result["state"] == "exited")   ret_detail["status"] = "Idle";
        else if (cmd_result["status"] == "created" || cmd_result["state"] == "created" ) ret_detail["status"] = "Starting";
        else ret_detail["status"] = "Undefined";
    } else {
        // command failed means container is not started
        ret_detail["status"]="Idle";
    }

    return ret_detail;
}

auto ContainerRuntimeAdapter::list() -> nlohmann::json{
    std::cout<< "NOT IMPLEMENTED: ContainerRuntimeAdapter::list() -> nlohmann::json" << std::endl;
    nlohmann::json ret;

    if (use_dobby) // Use Dobby
    {
        // Not implemented yet!
    }
    else // Crun
    {
        std::string command_list = "crun list";
        std::cout<<"ContainerRuntimeAdapter::list() -> \n   " << command_list <<std::endl;
    }
    
    return ret;
}
