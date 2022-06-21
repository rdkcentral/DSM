#include <iostream>

#include "dsm-controller.hpp"

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
   auto du = ee->find_deplyment_unit("sample.tar");
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
   dsm.command_handler_loop();
}

