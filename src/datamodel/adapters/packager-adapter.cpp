
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

#include "packager-adapter.hpp"
#include "../bridge/packager.hpp"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <filesystem>
namespace fs = std::filesystem;


#include "../../utils/file-system.hpp"

bool is_http(std::string uri)
{
   return (uri.substr(0,7) == "http://");
}

std::string convertToLocalUri(std::string uri)
{
   std::string localUri = uri;

   if (is_http(uri))
   {
      size_t length = uri.length();
      size_t   last = 0;

      bool  found = false;

      // Inefficient but easy to write! Replace with string ops support class
      for (size_t i=0;i<length;i++)
      {
         if (uri[i] =='/')
         {
            last = i; found = true;
         }
      }

      if (found)
      {
         localUri = uri.substr(last+1);

         std::cout << "Converted to local " << localUri;
      }
   }

   return localUri;
}

auto load_package_config (std::string dest, std::string id) -> nlohmann::json{
   nlohmann::json package_config;

   std::string localUri = convertToLocalUri(id);

   try{
      auto config_file = dest+localUri + ".json";   

      {
         std::ifstream du_file(config_file);
         du_file >> package_config;
      }
   } catch(const std::exception& e){
      return nlohmann::json::parse("{}");
   }
   return package_config;
}

auto save_package_config (std::string dest, std::string id, std::string uri, std::string state, std::string path="",std::string localUri="") -> nlohmann::json{
   {
      nlohmann::json package_data;
      package_data["id"] = id;
      package_data["uri"] = uri;
      package_data["state"] = state;
      // FIXME: ASSUMPTION HERE ON NAMING CONVENTION AND STRUCTURE OF PACKAGES

      // Only check for exec and save if path is assigned
      if (path.length() > 0)
      {
         package_data["path"] = path;
         package_data["exec"] = path_exists(path + "/config.json");
         try
         {
            std::ofstream package_file(dest + localUri + ".json");
            package_file << package_data <<std::endl;
         }
         catch (const std::exception &e)
         {
            std::cerr << e.what() << '\n';
         }
      }
      
      return package_data;
   }
}
auto delete_package_config (std::string dest, std::string id, std::string uri, std::string state, std::string path="",std::string localUri="") -> nlohmann::json
{
   nlohmann::json package_data;
   package_data["id"] = id;
   package_data["uri"] = uri;
   package_data["state"] = state;
   
   if (path.length() > 0)
   {
      auto file_name = path + ".tar.json";
      bool file_exist = path_exists(file_name);
      if (file_exist)
      {
         remove(file_name.c_str());
      }
   }
   return package_data;
}

PackagerAdapter::PackagerAdapter() : config("") {
   std::cout << "<<create>> PackagerAdapter(config=" << config << ")" << std::endl;
}

void PackagerAdapter::configure(nlohmann::json config) {
   std::cout << "PackagerAdapter->configure(" << config << ")" << std::endl;
   this->config = config;
   std::string destination_path {config["destination"]};
   if (!path_exists(destination_path)){
      throw std::invalid_argument(std::string("Destination path doesn't exist: ")+destination_path);
   }
}

auto PackagerAdapter::install(std::shared_ptr<PackageData> package,std::string id, std::string uri) -> nlohmann::json {
   // nlohmann::json ret;
   // although installation is mutliple subprocesses, adapters provide only install
   // requires only install
   auto repo = std::string(config["repo"]) + "/";
   auto dest = std::string(config["destination"]) + "/";

   std::string localUri = uri;

   std::cout << "PackagerAdapter::install(id=" << id << ", uri=" << uri << ") " << std::endl;
   std::cout << "   config=" << config << std::endl;

   auto package_config = load_package_config(dest, id);
   if (!package_config["state"].is_null() && package_config["state"]!="uninstalled"){
      std::cout << "    INSTALL ERROR: Package has been installed and is not unistalled."<<std::endl;
      return package_config;
   }

   // Package may be local file or come via http (should add https also). Replace code with support class!
   // Replace use of wget with libcurl or similar (curlpp)

   // DOWNLOAD
   std::string download_command;

   if (is_http(uri)) // Http Download
   {
      localUri = convertToLocalUri(uri);
 
      if (localUri != uri)
      {
         download_command = "wget -T 3 " + uri + " -O " + dest + "/" + localUri;
      }
   }
   else // Local file (Copy)
   {
      std::string download = "cp " + repo + uri + " " + dest;
   }

   save_package_config(dest, id, uri, "downloading");
   std::system(download_command.c_str());

   // Move to install (Note download errors not handled!)
   save_package_config(dest, id, uri, "installing");

   // INSTALL
   std::string install_command = "tar -xf " + dest + localUri + " -C " + dest;
   std::system(install_command.c_str());

   // Fix path of package here
   package->path = dest+localUri.substr(0, localUri.length()-4); // Assumes .tar --- FIXME

   // CLEAN UP
   std::string cleanup_command = "rm -f " + dest + localUri;
   std::system(cleanup_command.c_str());

   auto package_status = save_package_config(dest, id, uri, "installed", package->path,localUri);
   
   return package_status;
}

auto PackagerAdapter::uninstall(std::string id) -> nlohmann::json {   
   auto dest = std::string(config["destination"]) + "/";
   std::cout << "PackagerAdapter::uninstall(" << dest+ id + ".json"
             << ")" << std::endl;
   auto package_config = load_package_config(dest, id);

   std::string uri = package_config["uri"];

   save_package_config(dest, id, uri, "uninstalling");

   std::string path = package_config["path"];
   fs::remove_all(path);

   auto package_status = delete_package_config(dest, id, uri, "uninstalled",path,convertToLocalUri(id));
   return package_status;
}

auto PackagerAdapter::is_installed(std::string id) -> bool{
   auto dest = std::string(config["destination"]) + "/";
   auto package_config = load_package_config(dest, id);
   return ! (package_config["state"].is_null() || package_config["state"]=="uninstalled");
}

auto PackagerAdapter::list() -> nlohmann::json {
   auto dest = std::string(config["destination"]) + "/";
   auto ret = nlohmann::json::parse("[]");
   try
   {
      DIR* dirFile = opendir(dest.c_str());
      if (dirFile) {
         struct dirent* hFile;
         errno = 0;
         while ((hFile = readdir(dirFile)) != NULL) {
            if (!strcmp(hFile->d_name, ".")) continue;
            if (!strcmp(hFile->d_name, "..")) continue;
      if (strstr(hFile->d_name, ".json")) {
               nlohmann::json package_data;
               std::fstream package_file(dest + hFile->d_name);
               package_file >> package_data;
               ret.push_back(package_data);
            }
         }
         closedir(dirFile);
      }
   }
   catch(const std::exception& e)
   {
      std::cout << e.what() << std::endl;
   }
   return ret;
}

auto PackagerAdapter::check_executable(std::string uri) -> nlohmann::json { return ""; }
