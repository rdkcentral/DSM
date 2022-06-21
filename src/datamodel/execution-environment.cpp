#include "execution-environment.hpp"

#include <algorithm>
#include <iostream>

ExecutionEnvironment::ExecutionEnvironment(nlohmann::json config, std::shared_ptr<Packager> packager, std::shared_ptr<ContainerRuntime> runtime)
    : config(config), 
      packager(packager),
      runtime(runtime){
   std::cout << "<<create>> ExecutionEnvironment = " << config["name"] << std::endl;
}

auto ExecutionEnvironment::id() const -> unsigned int { return config["id"]; }

auto ExecutionEnvironment::name() const -> std::string { return config["name"]; }

auto ExecutionEnvironment::install(std::string uri) -> DeploymentUnit * {   
   auto package = packager->find_package(uri);
   if (package != nullptr && package->state != Packager::Uninstalled){
      std::cout << "   EE: Install: Package already installed" << std::endl;
      return nullptr;
   }
   auto du = find_deplyment_unit(uri);
   if (du == nullptr) {
      du = std::make_shared<DeploymentUnit>(this, uri, packager);
      du_list.push_back(du);
   }
   std::cout << "   EE: Install: back from vector" << std::endl;
   du->install();
   return &(*du);
}

auto ExecutionEnvironment::add_existing(PackageData &installed_package) -> DeploymentUnit * {
   std::cout << "   EE: add_existing:" << installed_package.ext_id << std::endl;
   auto du = std::make_shared<DeploymentUnit>(this, installed_package, packager);
   this->du_list.push_back(du);
   return &(*du);
}

auto ExecutionEnvironment::has_du_in_config(std::string uid) -> bool {
   for (auto conf_du : config["dus"]) {
      if (conf_du["UUID"] == uid) return true;
   }
   return false;
}

auto ExecutionEnvironment::find_deplyment_unit(const std::string uri) -> std::shared_ptr<DeploymentUnit> {
   auto elem = std::find_if(begin(du_list), end(du_list),
                            [=](std::shared_ptr<DeploymentUnit> du) { return du->get_uri() == uri; });
   if (elem == end(du_list)) return nullptr;
   return *elem;
}

auto ExecutionEnvironment::to_json() -> nlohmann::json {
   config["dus"] = nlohmann::json::parse("[]");
   for (auto &du : du_list) {
      config["dus"].push_back(du->to_json());
   }
   return config;
}
auto ExecutionEnvironment::is_default() -> bool { return config["default"]; }

auto ExecutionEnvironment::get_eu_list() -> std::vector<ExecutionUnit *> {
   std::vector<ExecutionUnit *> ret_vector;
   for (auto du: du_list){
      if (du->has_eu()){
         ret_vector.push_back(du->get_eu());
      }
   }
   return ret_vector;
}
auto ExecutionEnvironment::get_runtime() -> ContainerRuntime*{
   return runtime.get();
}
