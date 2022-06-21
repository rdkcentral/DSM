#ifndef __PACKAGER_HPP
#define __PACKAGER_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../adapters/packager-adapter.hpp"
#include "../../ext/json.hpp"
#include "../../utils/thread_worker.hpp"

struct PackageData;

class Packager {
   nlohmann::json config;
   std::vector<std::shared_ptr<PackageData> > packages_list;
   std::shared_ptr<ThreadWorker> worker;
   std::shared_ptr<PackagerAdapter> packager_adapter;

  public:
   enum PackageState { Undefined, New, Installing, Installed, InstallFail, Uninstalling, Uninstalled };
   static auto state_to_string(PackageState state) -> std::string;
   Packager();
   Packager(nlohmann::json config);
   auto append_package(std::string ext_id, std::string uri) -> bool;
   auto install(std::string ext_id) -> bool;
   auto uninstall(std::string ext_id) -> bool;
   void on_update(std::string ext_id, std::function<void(std::string, Packager::PackageState)> callback);
   auto find_package(std::string ext_id) -> std::shared_ptr<PackageData>;
   auto get_info(std::string ext_id) -> nlohmann::json;
   auto list() -> std::vector<std::shared_ptr<PackageData> >;
};

struct PackageData {
   std::string ext_id;
   std::string uri;
   std::string path;
   bool is_executable;
   Packager::PackageState state;
   std::function<void(std::string, Packager::PackageState)> update_callback;

   PackageData(std::string ext_id, std::string uri);
   PackageData(nlohmann::json package_json);
   void notify();
   auto to_json() -> nlohmann::json;
};

#endif
