
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

#include <iostream>

#include "dsm-controller.hpp"

#ifdef ENABLE_RBUS_PROVIDER
qweqwe
#include "rbus/dsm_rbus_provider.hpp"
#endif

void TestUse() {
   DSMController dsm;
   dsm.configure("../../src/dsm.config");

   std::cout << "  Has Environment 1 " << bool(dsm.find_execution_environment(1) != nullptr) << std::endl;
   std::cout << "  Has Environment 4 " << bool(dsm.find_execution_environment(4) != nullptr) << std::endl;

   auto eep = dsm.find_execution_environment(1);

   std::cout << "  Get Environment 1 " << dsm.find_execution_environment(1)->name() << std::endl;
   auto ee = dsm.find_execution_environment(1);
   // ee->install("hello_world.tar");
   ee->install("sample.tar");
   // ee->install("du_only.tar");
   auto du = ee->find_deployment_unit("sample.tar");
   du->uninstall();
}

int main() {
   std::cout << "DSM Main App" << std::endl;
   //    TestUse();
   DSMController dsm;
   const char *configFile;

   configFile = std::getenv("DSM_CONFIG_FILE");

   if (configFile == nullptr)
   {
      configFile = "../../src/dsm.config";
   }

   dsm.configure(configFile);

#ifdef ENABLE_RBUS_PROVIDER
   dsm_rbus_provider rbus_provider(dsm);
#endif
   

   dsm.command_handler_loop();
}

