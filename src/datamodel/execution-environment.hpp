#ifndef __EXECUTION_ENVIROMENT_HPP
#define __EXECUTION_ENVIROMENT_HPP

#include <memory>
#include <vector>

#include "../ext/json.hpp"
#include "deployment-unit.hpp"
#include "execution-unit.hpp"
#include "bridge/packager.hpp"
#include "bridge/container-runtime.hpp"

class DeploymentUnit;
class ExecutionUnit;

class ExecutionEnvironment {
   nlohmann::json config;
   std::shared_ptr<Packager> packager {nullptr};
   std::shared_ptr<ContainerRuntime> runtime {nullptr};
   std::vector<std::shared_ptr<DeploymentUnit> > du_list;
   
  public:
   ExecutionEnvironment(nlohmann::json config, std::shared_ptr<Packager> packager, std::shared_ptr<ContainerRuntime> runtime);
   ~ExecutionEnvironment() = default;

   auto id() const -> unsigned int;
   auto name() const -> std::string;
   auto install(std::string uri) -> DeploymentUnit *;
   auto add_existing(PackageData &installed_package) -> DeploymentUnit *;
   auto has_du_in_config(std::string uid) -> bool;
   auto find_deplyment_unit(std::string uri) -> std::shared_ptr<DeploymentUnit>;
   auto to_json() -> nlohmann::json;
   auto is_default() -> bool;
   auto get_eu_list() -> std::vector<ExecutionUnit *> ;
   auto get_runtime() -> ContainerRuntime*;
};

#endif
