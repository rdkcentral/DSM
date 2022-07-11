
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

#include "packager.hpp"

#include <algorithm>
#include <iostream>

void empty_callback(std::string uri, Packager::PackageState state) {}

PackageData::PackageData(std::string ext_id, std::string uri)
    : ext_id(ext_id), 
      uri(uri), 
      path(""),
      is_executable(false),
      state(Packager::New), 
      update_callback(empty_callback)
   {}
   
PackageData::PackageData(nlohmann::json data)
   : ext_id (data["id"]),
     uri(data["uri"]),
     path(data["path"]),
     is_executable(data["exec"]),
     state(Packager::Undefined),
     update_callback(empty_callback)

   {
      // nlohmann::json translate;
      std::map<std::string, Packager::PackageState> translate;
      translate["installing"] = Packager::Installing;
      translate["downloading"] = Packager::Installing;
      translate["installed"] = Packager::Installed;
      translate["uninstalling"] = Packager::Uninstalling;
      translate["uninstalled"] = Packager::Uninstalled;
      if (translate.find(data["state"]) != translate.end()){
         state = translate[data["state"]];
      }else{
         state = Packager::Undefined;
      }      
}

void PackageData::notify() { update_callback(uri, state); }

auto PackageData::to_json() -> nlohmann::json{
   nlohmann::json ret;
   ret["id"] = ext_id;
   ret["uri"] = uri;
   ret["path"] = path;
   ret["exec"] = is_executable;
   ret["state"] = Packager::state_to_string(state);
   return ret;
}


auto Packager::state_to_string(PackageState state) -> std::string{
   switch (state){
      case Undefined: return "Undefined";
      case New: return "New";
      case Installing: return "Installing";
      case Installed: return "Installed";
      case InstallFail: return "InstallFail";
      case Uninstalling: return "Uninstalling";
      case Uninstalled: return "Uninstalled";
   }
   return "Error. Packager::state_to_string() Unrecognized State";
}


Packager::Packager(nlohmann::json config="{}")
    : config(config), worker(std::make_shared<ThreadWorker>()), packager_adapter(std::make_shared<PackagerAdapter>()) {
   std::cout << "<<create>> Packager(config=" << config << ")" << std::endl;
   packager_adapter->configure(config[packager_adapter->name]);
   worker->start();
}

auto Packager::append_package(std::string ext_id, std::string uri) -> bool {
   // std::cout << "Packager.append(" << uri << ")" << std::endl;
   auto package = find_package(uri);
   if (nullptr != package) return false;
   packages_list.push_back(std::make_shared<PackageData>(ext_id, uri));
   return true;
}

auto Packager::install(std::string ext_id) -> bool {
   std::cout << "Packager.install(" << ext_id << ")" << std::endl;   
   if (packager_adapter->is_installed(ext_id)) {
      std::cout << "    package is installed already"<<std::endl;
      return false;
   }
   auto package = find_package(ext_id);
   // package is created in DU contructor. It must exists here already.
   if (nullptr == package) return false;

   worker->execute_async_task([=]() { 
      auto result = packager_adapter->install(package,ext_id, package->uri); 
      if (result["state"]=="installed") {
         package->is_executable = result["exec"];
         package->state = Packager::Installed;
      }
      else {
         package->state = Packager::Undefined;
      }
      package->notify();

   });
   package->state = Packager::Installing;
   package->notify();
   return true;
}

auto Packager::uninstall(std::string uid) -> bool {
   std::cout << "Packager.unistall(" << uid << ")" << std::endl;
   auto package = find_package(uid);
   std::cout << "Packager.unistall(" << uid << ")" << std::endl;
   if (nullptr == package) {
      std::cout << "  - package not found" << std::endl;
      return false;
   }
   if (Packager::Installed != package->state) {
      std::cout << "  - not installed. current state is: "<< Packager::state_to_string(package->state) << std::endl;
      return false;
   }
   // TODO add propper state management function (once core is stable and funcitonal)

   worker->execute_async_task([=]() { 
      auto result = packager_adapter->uninstall(uid); 
      if (result["state"]=="uninstalled") package->state = Packager::Uninstalled;
      else package->state = Packager::Undefined;
      package->notify();
   });

   package->state = Packager::Uninstalling;
   package->notify();
   return true;
}

auto Packager::find_package(std::string ext_id) -> std::shared_ptr<PackageData> {
   auto package = std::find_if(begin(packages_list), end(packages_list),
                               [=](std::shared_ptr<PackageData> elem) { return elem->ext_id == ext_id; });
   if (package == end(packages_list)) return nullptr;
   return *package;
}

void Packager::on_update(std::string ext_id, std::function<void(std::string, Packager::PackageState)> callback) {
   auto package = find_package(ext_id);
   if (nullptr == package) return;
   package->update_callback = callback;
   package->notify();
}

auto Packager::get_info(std::string ext_id) -> nlohmann::json { 
   auto package = find_package(ext_id);
   if (nullptr == package) return "";

// TODO: Update internal state and return packager information to DEployment Unit "is executable"
   return package->to_json(); 
}

auto Packager::list() -> std::vector<std::shared_ptr<PackageData> > {
   auto packages_list_json = packager_adapter->list();
   for (auto &package_it : packages_list_json.items()) {
      auto package_json = package_it.value();
      if (find_package(package_json["id"])) continue;
      // packages_list.push_back(std::make_shared<PackageData>(package["id"], package["uri"]));
      packages_list.push_back(std::make_shared<PackageData>(package_json));
   }
   return packages_list;
}
