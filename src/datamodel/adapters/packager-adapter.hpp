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
};

#endif
