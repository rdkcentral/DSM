
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

#include <Dobby/DobbyProxy.h>
#include <Dobby/IpcService/IpcFactory.h>

// -----------------------------------------------------------------------------
/**
 * @brief
 *
 *
 *
 */
static int32_t getContainerDescriptor(const std::shared_ptr<IDobbyProxy> &dobbyProxy,
                                      const std::string &id)
{
    // get a list of the containers so can match id with descriptor
    const std::list<std::pair<int32_t, std::string>> containers = dobbyProxy->listContainers();
    for (const std::pair<int32_t, std::string> &container : containers)
    {
        char strDescriptor[32];
        sprintf(strDescriptor, "%d", container.first);

        if ((id == strDescriptor) || (id == container.second))
        {
            return container.first;
        }
    }

    return -1;
}
static std::string getContainerState(const std::shared_ptr<IDobbyProxy> &dobbyProxy, int32_t descriptor)
{
    std::string state;
    switch (dobbyProxy->getContainerState(descriptor))
    {
    case int(IDobbyProxyEvents::ContainerState::Invalid):
        state = "Idle";
        break;
    case int(IDobbyProxyEvents::ContainerState::Starting):
        state = "Starting";
        break;
    case int(IDobbyProxyEvents::ContainerState::Running):
        state = "Active";
        break;
    case int(IDobbyProxyEvents::ContainerState::Stopping):
        state = "Stopping";
        break;
    case int(IDobbyProxyEvents::ContainerState::Paused):
        state = "Stopping";
        break;
    default:
        state = "error";
    }

    return state;
}

ContainerRuntimeAdapter::ContainerRuntimeAdapter()
{
#ifdef CONTAINER_RUNTIME_USE_CRUN
    use_dobby = false;
#else
    use_dobby = true;
    mDobbyProxy = nullptr;
    mIpcService = AI_IPC::createIpcService("unix:path=/var/run/dbus/system_bus_socket", "org.rdk.dobby.processcontainers");

    if (mIpcService)
    {
        mIpcService->start();
        mDobbyProxy = std::make_shared<DobbyProxy>(mIpcService, DOBBY_SERVICE, DOBBY_OBJECT);
        if (!mDobbyProxy)
        {
            std::cerr << "mDobbyProxy is null" << std::endl;
        }
    }
#endif
}

void ContainerRuntimeAdapter::configure(nlohmann::json config)
{
    std::cout << "ContainerRuntimeAdapter::configure(nlohmann::json config)" << config << std::endl;
}
auto ContainerRuntimeAdapter::start(std::string id, std::string path) -> nlohmann::json
{
    std::cout << "ContainerRuntimeAdapter::start(std::string id, std::string uri) -> nlohmann::json" << id << "," << path << std::endl;
    nlohmann::json ret;
    
    // this is only the minimum brute force implementation for CLI

    if (use_dobby) // Use Dobby
    {
        if (mDobbyProxy)
        {
            int32_t descriptor = mDobbyProxy->startContainerFromBundle(id, path, {});
            std::cout << "startContainerFromBundle id=" << id.c_str() << " descriptor=" << descriptor << std::endl;
        }
        else
        {
            std::cerr << "ContainerRuntimeAdapter::start mDobbyProxy is null" << std::endl;
        }
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
auto ContainerRuntimeAdapter::stop(std::string id) -> nlohmann::json
{
    std::cout << "ContainerRuntimeAdapter::stop(std::string id) -> nlohmann::json" << id << std::endl;
    nlohmann::json ret;

    if (use_dobby) // Use Dobby
    {
        if (mDobbyProxy)
        {
            int32_t descriptor = getContainerDescriptor(mDobbyProxy, id);
            if (descriptor > 0)
            {
                bool stoppedSuccessfully = mDobbyProxy->stopContainer(descriptor);
                if (stoppedSuccessfully)
                {
                    std::cout << "stopContainer id= " << id.c_str() << " stoppedSuccessfully" << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "ContainerRuntimeAdapter::stop mDobbyProxy is null" << std::endl;
        }
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

auto ContainerRuntimeAdapter::detail(std::string id) -> nlohmann::json
{
    //std::cout << "ContainerRuntimeAdapter::detail id=" << id.c_str() << std::endl;
    nlohmann::json ret;

    std::string result;
    int ret_code;

    std::string command_state;
    nlohmann::json ret_detail {};

    if (use_dobby) // Use Dobby
    {
        if (mDobbyProxy)
        {
            int32_t descriptor = getContainerDescriptor(mDobbyProxy, id);
            if (descriptor > 0)
            {
                result = getContainerState(mDobbyProxy, descriptor);
                ret_detail["status"] = result;
            }
            else
            {
                result = "error";
                ret_detail["status"] = "Idle";
            }
        }
        else
        {
            std::cerr << "ContainerRuntimeAdapter::detail mDobbyProxy is null" << std::endl;
            result = "error";
            ret_detail["status"] = "Idle";
        }
        
        return ret_detail;
    }
    else // Use Crun
    {
        command_state = "crun state "+id;
        std::tie(ret_code, result) = execute_command(command_state);
    
        // Check for no JSON (Dobby does this with no active container)
        if (result.substr(0,5) == "error")
        {
            ret_detail["status"] = "Idle";
            // If no container, dobby returns an error AND no JSON
            return ret_detail; // Assume idle
        }

        if (ret_code == 0){
            // Translate data from execution env to TR-Datamodel
            auto cmd_result = nlohmann::json::parse(result, nullptr, false);

            // ret_detail["state"] = cmd_result["status"];
            // Test for JSON format of both crun & dobby (Why are these the same!?)
            if (cmd_result["status"] == "running" || cmd_result["state"] == "running") 
            {
                ret_detail["status"] = "Active";
            }
            else if (cmd_result["status"] == "stopped" || cmd_result["state"] == "stopped")  
            {
                ret_detail["status"] = "Idle";
            }
            else if (cmd_result["status"] == "exited"  || cmd_result["state"] == "exited")  
            {
            ret_detail["status"] = "Idle";  
            }
            else if (cmd_result["status"] == "created" || cmd_result["state"] == "created" ) 
            {
                ret_detail["status"] = "Starting";
            }
            else 
            {   
                ret_detail["status"] = "Undefined";
            }

        } else {
            // command failed means container is not started
            ret_detail["status"] = result;
        }
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
