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