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
        ExecutionUnit &operator= (const ExecutionUnit &) = delete;

        void start();
        void stop();
        auto get_state() -> ContainerRuntime::ContainerState;
        auto get_detail() -> nlohmann::json;
        auto get_uid() -> std::string;
};

#endif

