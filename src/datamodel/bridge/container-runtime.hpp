#ifndef __CONTAINER_RUNTIME_HPP
#define __CONTAINER_RUNTIME_HPP

/* comment is FOR REFERENCE ONLY
DOBBY_DBUS_METHOD(startFromBundle);
    DOBBY_DBUS_METHOD(stop);
    DOBBY_DBUS_METHOD(pause);
    DOBBY_DBUS_METHOD(resume);
    DOBBY_DBUS_METHOD(exec);
    DOBBY_DBUS_METHOD(list);
    DOBBY_DBUS_METHOD(getState);
    DOBBY_DBUS_METHOD(getInfo);
*/

#include <string>

#include "../adapters/container-runtime-adapter.hpp"
#include "../../ext/json.hpp"
#include "../../utils/thread_worker.hpp"

struct ContainerData;

class ContainerRuntime {
  nlohmann::json config;
   std::vector<std::shared_ptr<ContainerData> > containers_list;
   std::shared_ptr<ThreadWorker> worker;
   std::shared_ptr<ContainerRuntimeAdapter> container_adapter;

  public:
   enum ContainerState { Undefined, Idle, Starting, Active, Stopping };

   ContainerRuntime(const nlohmann::json config = "{}");
   auto start(const std::string &id, const std::string &bundle_path) -> bool;
   auto stop(std::string id) -> bool;
   auto pause(std::string id) -> bool;
   auto resume(std::string id) -> bool;
   auto getState(std::string id) -> std::string;
   auto getInfo(std::string id) -> nlohmann::json;
   auto list() -> std::vector<std::shared_ptr<ContainerData> >;
   auto find_container(std::string ext_id) -> std::shared_ptr<ContainerData>;
   //  auto registerStateChnageHandler() -> int;
};

struct ContainerData{
  std::string ext_id;
  ContainerRuntime::ContainerState state;
  std::function<void(std::string, ContainerRuntime::ContainerState)> update_callback;

  ContainerData(std::string ext_id);
  void notify();
};

#endif
