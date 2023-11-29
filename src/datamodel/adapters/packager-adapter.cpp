
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
#include <ftw.h>


#include "../../utils/file-system.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

bool is_http(std::string uri)
{
   return (uri.substr(0,7) == "http://");
}

void PackagerAdapter::fork_exe(char* path, char *const args[], const std::function<void(void)>& fn_callback) 
{
  std::cout << "fork_exe path=" << path << std::endl;
  pid_t pid;
  int status;
  pid_t ret;
  //char **env = NULL;
  //extern char **environ;
 
  /* ... Sanitize arguments ... */
 
  pid = fork();
  if (pid == -1) {
    /* Handle error */
  } else if (pid != 0) {
    while ((ret = waitpid(pid, &status, 0)) == -1) {
      if (errno != EINTR) {
        /* Handle error */
        break;
      }
    }
    if ((ret == 0) ||
        !(WIFEXITED(status) && !WEXITSTATUS(status))) {
      /* Report unexpected child status */
    }
    fn_callback();
  } else {
    /* ... Initialize env as a sanitized copy of environ ... */
    int result = execv(path, args);
    std::cout << "execve path=" << path << ", result=" << result << std::endl;
    if (result == -1) {
      /* Handle error */
      _Exit(127);
    }
  }
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

         std::cout << "Converted to local " << localUri << std::endl;
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
      std::cout << "save_package_config dest=" << dest.c_str() << ", state=" << state.c_str() << std::endl;
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
      if(!create_directory(destination_path))
         throw std::invalid_argument(std::string("Destination path doesn't exist and unable to create directory: ")+destination_path);

   }
}

void PackagerAdapter::wget_callback_success(std::shared_ptr<PackageData> package, std::string id, std::string uri, std::string dest, std::string localUri) 
{
   std::cout << "wget_callback_success" << std::endl;

   // INSTALL

   std::function<void(void)> fn_install_callback_success = [=]() {
     // CLEAN UP
     std::string delete_file_command = dest + localUri;
     std::remove(delete_file_command.c_str());
     std::cout << "Delete file=" << delete_file_command.c_str() << std::endl;
   };

   std::string arg1 = "-xf";
   std::string arg2 = dest + localUri;
   std::string arg3 = "-C";
   std::string arg4 = dest;

   char * argv_list[] = {(char*)"tar",(char*)arg1.c_str(),(char*)arg2.c_str(),
                        (char*)arg3.c_str(), (char*)arg4.c_str(), NULL};

   fork_exe((char*)"/bin/tar", argv_list, fn_install_callback_success);
}

void PackagerAdapter::fork_exe_wget(std::shared_ptr<PackageData> package, std::string id, std::string uri, std::string dest, std::string localUri) 
{
   std::function<void(void)> fn_wget_callback = [=]()
   {
      wget_callback_success(package, id, uri, dest, localUri);
   };

   std::string arg1 = "-T";
   std::string arg2 = "3";
   std::string arg3 = uri;
   std::string arg4 = "-O";
   std::string arg5 = dest + "/" + localUri;

   char * argv_list[] = {(char*)"wget",(char*)arg1.c_str(),(char*)arg2.c_str(), (char*)arg3.c_str(), 
   (char*)arg4.c_str(), (char*)arg5.c_str(), NULL};

   fork_exe((char*)"/usr/bin/wget", argv_list, fn_wget_callback);
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
   
   if (is_http(uri)) // Http Download
   {
      localUri = convertToLocalUri(uri);
 
      save_package_config(dest, id, uri, "downloading");

      // Fix path of package here
      

      fork_exe_wget(package, id, uri, dest, localUri);

      // Move to install (Note download errors not handled!)
      auto package_status = save_package_config(dest, id, uri, "installing");

      package->path = dest+localUri.substr(0, localUri.length()-4); // Assumes .tar --- FIXME

      package_status = save_package_config(dest, id, uri, "installed", package->path,localUri);


      return package_status;
   }

   return package_config;
}

static int ftw_callback(const char *path, const struct stat *sb, int flag, struct FTW *buf){
   int ret = remove(path);
   if (ret) { //assume empty dir
      ret = unlink(path);
   }
   return ret;
}

auto PackagerAdapter::uninstall(std::string id) -> nlohmann::json {   
   auto dest = std::string(config["destination"]) + "/";
   std::cout << "PackagerAdapter::uninstall(" << dest+ id + ".json"
             << ")" << std::endl;
   auto package_config = load_package_config(dest, id);

   std::string uri = package_config["uri"];

   save_package_config(dest, id, uri, "uninstalling");

   std::string path = package_config["path"];
   nftw(path.c_str(), ftw_callback, 64, FTW_DEPTH | FTW_PHYS);
   remove(path.c_str());

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
