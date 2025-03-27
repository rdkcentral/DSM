#ifndef DSM_RBUS_PROVIDER_H
#define DSM_RBUS_PROVIDER_H

#include <librbusprovider/provider.h>
#include <librbusprovider/table.h>

#include "../ext/json.hpp"

class DSMController; //forward deceleration

class dsm_rbus_provider final : public rbus_provider
{
public:
   explicit dsm_rbus_provider(DSMController & controller);
   ~dsm_rbus_provider();

   dsm_rbus_provider() = delete;

   //DU methods
   static void installDU(rbusObject_t inParams, rbusMethodAsyncHandle_t aupdate_du_entrysyncHandle);
   static void uninstall(rbusObject_t inParams, rbusMethodAsyncHandle_t asyncHandle, rbusObject_t& outParams);
   static void updateDU (rbusObject_t inParams, rbusMethodAsyncHandle_t asyncHandle, table_row& DU_row);

   //EE methods
   static rbusError_t SetRunLevel(table_row& row, rbusObject_t inParams);
   static rbusError_t SetRequestedState(table_row& row, rbusObject_t inParams, rbusObject_t& outParams);

   //EE mirroring public items (most need to be accessed by a static function)
   std::map<int,nlohmann::json> ee_cache;
   std::map<std::string,nlohmann::json> du_cache;
   std::map<std::string,nlohmann::json> eu_cache;

   bool get_worker_active();
   void update_ee_entry(int index, nlohmann::json& data);

   void update_du_entry(std::string url, nlohmann::json& data);

   void update_eu_entry(std::string url, nlohmann::json& data);

private:
   static bool isValidURL(std::string url);
   rbusCallbackTable_t local_method_table = {nullptr,nullptr,nullptr,nullptr,nullptr,reinterpret_cast<void*>(dsm_rbus_provider::methodHandler)};
   
   static rbusError_t methodHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);
   static void stateChangeHandler(rbusHandle_t handle, rbusEvent_t const* event,rbusEventSubscription_t* subscription);

   //event components for reflecting rbus->dsm
   static void ee_external_change_handler(rbusHandle_t handle, rbusEvent_t const* event,rbusEventSubscription_t* subscription);
   static void du_external_change_handler(rbusHandle_t handle, rbusEvent_t const* event,rbusEventSubscription_t* subscription);
   static void eu_external_change_handler(rbusHandle_t handle, rbusEvent_t const* event,rbusEventSubscription_t* subscription);

   static rbusError_t dsmTableSetHandler(rbusHandle_t handle, rbusProperty_t property,rbusSetHandlerOptions_t* options);
   rbusCallbackTable_t rbus_table_dsm_read_write {rbus_table::tableGetHandler, dsm_rbus_provider::dsmTableSetHandler, nullptr, nullptr, nullptr, nullptr};

   //polling components for reflecting dsm->rbus
   bool workers_active = true;
   std::unique_ptr<std::thread> ee_worker {nullptr};
   static void ee_worker_fn(dsm_rbus_provider *parent);

   std::unique_ptr<std::thread> du_eu_worker {nullptr};
   static void du_eu_worker_fn(dsm_rbus_provider *parent);
   static void du_update_fn(dsm_rbus_provider *parent);
   static void eu_update_fn(dsm_rbus_provider *parent);

   static std::map<int,uint32_t> rbus_ee_instance_map;
   void ee_sync_table_init();
   bool add_ee_entry(int index, nlohmann::json& data);

   static std::map<std::string,uint32_t> rbus_du_instance_map;
   void du_sync_table_init();
   bool add_du_entry(std::string url, nlohmann::json& data);
   void remove_du_entry(std::string url);

   static std::map<std::string,uint32_t> rbus_eu_instance_map;
   void eu_sync_table_init();
   bool add_eu_entry(std::string, nlohmann::json& data);
   void remove_eu_entry(std::string uid);

   //due to the fact that handlers and methods need to be static because rbus is using function pointers with a specific template
   //this needs to be a static pointer, when it would be better implemented as a non-static reference
   static DSMController * DSM_ref;
};
#endif
