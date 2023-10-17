#include "dsm_rbus_provider.hpp"
#include <librbusprovider/utility.h>
#include <librbusprovider/rbus_macros.h>

#include "../dsm-controller.hpp"

#include <thread>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <mutex>
#include <regex>

using namespace utility_functions;

DSMController * dsm_rbus_provider::DSM_ref = nullptr;

std::map<int,uint32_t> dsm_rbus_provider::rbus_ee_instance_map = {};
std::map<std::string,uint32_t> dsm_rbus_provider::rbus_du_instance_map = {};
std::map<std::string,uint32_t> dsm_rbus_provider::rbus_eu_instance_map = {};

bool dsm_rbus_provider::get_worker_active() { return workers_active; }

dsm_rbus_provider::dsm_rbus_provider(DSMController & controller)
: rbus_provider("dsm_tr181_provider") {
   DSM_ref = &controller;
   registerPath( DEVICE_SM_PREFIX "InstallDU()",
      rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_METHOD,local_method_table}));

   registerPath( DEVICE_SM_PREFIX "DUStateChange!",
      rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_EVENT,rbus_callback_tables::rbus_event}));

   // internal (non tr-181) event for reflecting rbus-side changes into dsm's internal state.
   registerPath("ee_external_change",
      rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_EVENT,rbus_callback_tables::rbus_event}));
   registerPath("du_external_change",
      rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_EVENT,rbus_callback_tables::rbus_event}));
   registerPath("eu_external_change",
      rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_EVENT,rbus_callback_tables::rbus_event}));

   rbus_table DU ("DeploymentUnit",{
      //{"URL",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,dsm_rbus_provider::rbus_table_dsm_read_write}) },
      {"URL",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"Status",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"ExecutionUnitList",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_write}) },
      {"ExecutionEnvRef",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"Uninstall()",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_METHOD,rbus_callback_tables::rbus_table_generic_method}) },
      {"Update()",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_METHOD,rbus_callback_tables::rbus_table_generic_method}) }
   });
   tables.insert({DU.getName(), DU});

   rbus_table EU ("ExecutionUnit",{
      {"Name",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"Status",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"SetRequestedState()",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_METHOD,rbus_callback_tables::rbus_table_generic_method}) }
   });
   tables.insert({EU.getName(), EU});

   rbus_table EE ("ExecEnv",{
      {"Enable",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,dsm_rbus_provider::rbus_table_dsm_read_write}) },
      {"Status",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"Name",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"InitialRunLevel",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,dsm_rbus_provider::rbus_table_dsm_read_write}) },
      {"CurrentRunLevel",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_PROPERTY,rbus_callback_tables::rbus_table_generic_read_only}) },
      {"SetRunLevel()",rbus_data({nullptr,rbusElementType_t::RBUS_ELEMENT_TYPE_METHOD,rbus_callback_tables::rbus_table_generic_method}) }
   });
   tables.insert({EE.getName(), EE});

   registerTables();

   ee_sync_table_init();
   du_sync_table_init();
   eu_sync_table_init();
   //ee_worker = std::make_unique<std::thread>(ee_worker_fn,this);
   du_eu_worker = std::unique_ptr<std::thread>(new std::thread(du_eu_worker_fn,this));
   rbusEvent_Subscribe(rbusHandle,"ee_external_change",ee_external_change_handler,nullptr,0);
   rbusEvent_Subscribe(rbusHandle,"du_external_change",du_external_change_handler,nullptr,0);
   rbusEvent_Subscribe(rbusHandle,"eu_external_change",eu_external_change_handler,nullptr,0);

}

dsm_rbus_provider::~dsm_rbus_provider() {
   //Every DU had its elements subscribed to the handler. Unsubscribe each from it on close
   rbus_table& DU = tables.at("DeploymentUnit");

   //NOTE: if mirroring of tables is achieved in a different way to the mock , as we are in the same program space as the deployment units in this case , this may change and unsubscribing may not be required
   for(auto& row : DU.rows)  {
      for(const auto & ref : DU.GetTemplateRow()) {
         const auto & value = ref.first;
         //const auto & data = ref.second;
         std::string path = std::string(DEVICE_SM_PREFIX "DeploymentUnit.") + std::to_string(row.first) + std::string(".") + value;
         rbusEvent_Unsubscribe(rbusHandle,path.c_str());
      }
   }

   workers_active = false;
   //ee_worker->join();
   du_eu_worker->join();

   rbusEvent_Unsubscribe(rbusHandle,"ee_external_change");
   rbusEvent_Unsubscribe(rbusHandle,"du_external_change");
   rbusEvent_Unsubscribe(rbusHandle,"eu_external_change");

}

void dsm_rbus_provider::installDU(rbusObject_t inParams, rbusMethodAsyncHandle_t asyncHandle) 
{
   rbusError_t ec = RBUS_ERROR_SUCCESS;
   rbusValue_t stat;
   rbusValue_Init(&stat);

   rbusValue_t urlval = rbusObject_GetValue(inParams,"URL");
   if( urlval!=nullptr && rbusValue_GetType(urlval)==rbusValueType_t::RBUS_STRING ) 
   {
      const char* url = rbusValue_GetString(urlval,nullptr);

      //url is required
      if (url != nullptr) 
      {
         bool is_url_valid = isValidURL(url);
         if (!is_url_valid)
         {
            rbusValue_SetString(stat,"Failed: Invalid URL given");
            ec = RBUS_ERROR_INVALID_INPUT;
         }
         else
         {
            std::mutex installDU_Mutex;
            std::lock_guard<std::mutex> lock (installDU_Mutex);

            //convert the inParams to the json format expected by DSMController
            auto params = nlohmann::json::parse("{}", nullptr, false);
            params["uri"] = std::string(url);
            
            rbusValue_t eeref_val = rbusObject_GetValue(inParams,"ExecutionEnvRef");
            if( eeref_val!=nullptr && rbusValue_GetType(eeref_val)==rbusValueType_t::RBUS_STRING ) 
            {
               const char* ee_ref = rbusValue_GetString(eeref_val,nullptr);
               if (ee_ref != nullptr)
               {
                  params["ee_name"] = std::string(ee_ref);
               }
            }
            else /*use default if argument not given */ {
               params["ee_name"] = "default";
            }
            //TODO handle other optional parameters (dsm only uses ee and uri currently)
            std::cout << "dsm_rbus_provider::installDU before du_install" << std::endl;   
            auto dsm_ret_val = DSM_ref->du_install( params);
            auto json_str = dsm_ret_val.dump();
            rbusValue_SetString(stat,json_str.c_str());
         }
      }
    }
    else
    {
        rbusValue_SetString(stat,"Failed: No URL given");
        ec = RBUS_ERROR_INVALID_INPUT;
    }
   rbusObject_t outParams;
   rbusObject_Init(&outParams,nullptr);
   rbusObject_SetValue(outParams,"Status",stat);
   rbusValue_Release(stat);

   //async handle passthrough
   rbusValue_t handle = rbusObject_GetValue(inParams,"uspasynchandle");
   if(handle!=nullptr) { rbusObject_SetValue(outParams,"uspasynchandle",handle); }

   rbusError_t sent = rbusMethod_SendAsyncResponse(asyncHandle,ec,outParams);
   if(sent != RBUS_ERROR_SUCCESS) { std::cerr << "Failed to return result from async method\n"; } 

   rbusObject_Release(inParams);
   rbusObject_Release(outParams);
}

void dsm_rbus_provider::uninstall(rbusObject_t inParams, rbusMethodAsyncHandle_t asyncHandle, rbusObject_t& outParams) 
{
   rbusValue_t ret;
   rbusValue_Init(&ret);

   rbusValue_t const reqIndex = rbusObject_GetValue(inParams,"index");
   if( reqIndex!=nullptr && rbusValue_GetType(reqIndex)==rbusValueType_t::RBUS_UINT32 ) 
   {
      auto index = rbusValue_GetUInt32(reqIndex);
      for (const auto& ref : rbus_du_instance_map)
      {
         const auto & key = ref.first;
         const auto & value = ref.second;
         if (value == index)
         {
            bool is_url_valid = isValidURL(key);
            if (is_url_valid)
            {
               std::mutex uninstallDU_Mutex;
               std::lock_guard<std::mutex> lock (uninstallDU_Mutex);

               //convert the inParams to the json format expected by DSMController
               auto params = nlohmann::json::parse("{}", nullptr, false);
               params["uuid"] = key;
               auto dsm_ret_val = DSM_ref->du_uninstall( params);
               auto ret_json_str = dsm_ret_val.dump();
               rbusValue_SetString(ret,ret_json_str.c_str());
               rbusObject_SetValue(outParams,"Ret",ret);
               //async handle passthrough
               rbusValue_t handle = rbusObject_GetValue(inParams,"uspasynchandle");
               if(handle!=nullptr) { rbusObject_SetValue(outParams,"uspasynchandle",handle); }
               return;
            }
         }
      }
   }
   
   rbusValue_SetString(ret,"Failed: No URL found");
   rbusObject_SetValue(outParams,"Ret",ret);
}

rbusError_t dsm_rbus_provider::SetRunLevel(table_row& row, rbusObject_t inParams) {
   rbusValue_t const reqRL = rbusObject_GetValue(inParams,"RequestedRunLevel");
   if(reqRL!=nullptr && rbusValue_GetType(reqRL)==rbusValueType_t::RBUS_INT32) {
      int32_t newrl = rbusValue_GetInt32(reqRL);
      if(newrl>=-1&&newrl<=65535) {
         row["CurrentRunLevel"].rbus_int = newrl;
         return RBUS_ERROR_SUCCESS;
      }
   }
   return RBUS_ERROR_INVALID_INPUT;
}
rbusError_t dsm_rbus_provider::SetRequestedState(table_row& row, rbusObject_t inParams, rbusObject_t& outParams) {
   rbusValue_t ret;
   rbusValue_Init(&ret);

   rbusValue_t const reqState = rbusObject_GetValue(inParams,"RequestedState");
    if( reqState!=nullptr && rbusValue_GetType(reqState)==rbusValueType_t::RBUS_STRING ) {
        const char* state = rbusValue_GetString(reqState,nullptr);
        if (state != nullptr) {
            if (strcmp(state,"Idle") == 0 || strcmp(state,"Active") == 0)
            {
               auto uid = row["Name"].rbus_string;
               nlohmann::json uid_json;
               uid_json["uid"] = uid;
               nlohmann::json ret_json;
                  
               if (strcmp(state,"Active") == 0) 
               {
                  ret_json = dsm_rbus_provider::DSM_ref->eu_start(uid_json);
               }
               else if (strcmp(state,"Idle") == 0) 
               {
                  ret_json = dsm_rbus_provider::DSM_ref->eu_stop(uid_json);
               }
               auto ret_json_str = ret_json.dump();
               rbusValue_SetString(ret,ret_json_str.c_str());
               rbusObject_SetValue(outParams,"Ret",ret);
               return (rbusError_t::RBUS_ERROR_SUCCESS);
            }
            else
            {
               return (rbusError_t::RBUS_ERROR_INVALID_INPUT);           
            }
        }
    }
    return (rbusError_t::RBUS_ERROR_INVALID_INPUT);
}

bool dsm_rbus_provider::isValidURL(std::string url)
{
  // Regex to check valid URL
  const std::regex pattern("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
 
  // If the URL
  // is empty return false
  if (url.empty())
  {
     return false;
  }
 
  // Return true if the URL
  // matched the ReGex
  if(std::regex_match(url, pattern))
  {
    return true;
  }
  else
  {
    return false;
  }
}

rbusError_t dsm_rbus_provider::methodHandler(UNUSED_CHECK rbusHandle_t handle, char const* methodName, rbusObject_t inParams, UNUSED_CHECK rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle) {
   if (strcmp(methodName,"Device.SoftwareModules.InstallDU()") == 0) {
      rbusObject_Retain(inParams);
      try {
         std::thread(dsm_rbus_provider::installDU, inParams,asyncHandle).detach();
      }
      catch(...) {
         std::cerr<<"Failed to spawn thread\n";
         return RBUS_ERROR_BUS_ERROR;
      }

      return RBUS_ERROR_ASYNC_RESPONSE;
   }

   return RBUS_ERROR_BUS_ERROR;
}

void dsm_rbus_provider::stateChangeHandler(UNUSED_CHECK rbusHandle_t handle, rbusEvent_t const* event,UNUSED_CHECK rbusEventSubscription_t* subscription) {   
   rbusEvent_t duevent {};
   rbusObject_t data;
   rbusValue_t val,faultc,faultstr,resolved,startTime,completeTime;
   static bool is_update = false;

   std::string path(event->name), field, tmp;      
   std::stringstream buf (path);

   //split the path into the DU ref and the field that has changed (stored in path and field)
   std::vector<std::string> parts = splitStringToVector(path);

   field = parts.back();
   parts.pop_back();
   buf.clear();
   buf.str(std::string()); //reset buffer
   buf << parts.front();
   parts.erase(parts.begin());
   for(auto const & str : parts) { buf << '.' << str; }
   path = buf.str();

   rbusObject_Init(&data,nullptr);
   rbusValue_Init(&val);

   rbusValue_SetString(val,path.c_str());
   rbusObject_SetValue(data,"DeploymentUnitRef",val);
   rbusValue_Release(val);

   rbusObject_SetValue(data,"CurrentState",rbusObject_GetValue(event->data,"value"));

   const char * before = rbusValue_GetString(rbusObject_GetValue(event->data,"oldValue"),nullptr) ;
   const char * after = rbusValue_GetString(rbusObject_GetValue(event->data,"value"),nullptr);

   for (rbusValue_t * value : {&val,&faultc,&faultstr,&resolved,&startTime,&completeTime}) { rbusValue_Init(value); }

   // TODO set to unknown time according to the spec, needs to get start time somehow
   // can't be function arg, or by event data that triggered this call , so possibly a 
   //static object needed to store the start time of an install/uninstall/status
   //or reading something set when the actual install is done (which it isn't here in the mock)
   rbusValue_SetString(startTime,"0001-01-01T00:00:00Z"); 

   rbusDateTime_t end_time {};
   time_t now = time(nullptr);
   // This copy can be removed when UnMarshall sets invalue to be a const
   rbusDateTime_t tmp_copy = *tmp; 
   struct tm val_as_tm;
   rbusValue_UnMarshallRBUStoTM(&val_as_tm, &tmp_copy);
   strftime(num_str, sizeof(num_str), "%Y-%m-%dT%H:%M:%SZ", &val_as_tm);

   rbusValue_SetTime(completeTime,&end_time);

   if(field.compare("Status")==0) {
      if(strcmp(before,"Installing")==0) {
         //started as installing so the operation is Install or Update
         if (!is_update) {
            rbusValue_SetString(val,"Install");
            if(strcmp(after,"Installed")==0) {
               //success
               rbusValue_SetUInt16(faultc,0);
               rbusValue_SetString(faultstr,"");
               rbusValue_SetBoolean(resolved,true);
            }
            else {
               //failure
               rbusValue_SetUInt16(faultc,9001);
               rbusValue_SetString(faultstr,"Install Failed");
               rbusValue_SetBoolean(resolved,true); //this may or may not be true when the real deployment units are installed if any dependencies fail to resolve
            }
         }
         else {
            rbusValue_SetString(val,"Update");
            is_update = false;
            rbusValue_SetUInt16(faultc,0);
            rbusValue_SetString(faultstr,"");
            rbusValue_SetBoolean(resolved,true);
         }

      }
      else /*implict if (strcmp(before,"Installed")==0)*/ {
            //update doesn't affect status, so only other option is uninstall 
            rbusValue_SetString(val,"Uninstall");
            rbusValue_SetBoolean(resolved,true); //data model says to set this always to true for uninstall
            if(strcmp(after,"Uninstalled")==0) {
               //success
               rbusValue_SetUInt16(faultc,0);
               rbusValue_SetString(faultstr,"");
            }
            else if(strcmp(after,"Uninstalling")==0) {
               rbusValue_SetUInt16(faultc,0);
               rbusValue_SetString(faultstr,"");
            }
            else if (strcmp(after,"Installing")==0) /*Installed -> Installing */ {
               //this is the first step of the Update() command.
               is_update = true;
               rbusValue_SetString(val,"Update");
               rbusValue_SetUInt16(faultc,0);
               rbusValue_SetString(faultstr,"");
            }
            else {  //failure
               rbusValue_SetUInt16(faultc,9001);
               rbusValue_SetString(faultstr,"Uninstall Failed");
            }
      }
   }
   else if (field.compare("URL")==0) {
      //the URL should only change when an update() is ran which will also change the status. 
      //to prevent the event firing twice in quick succession and to give the impression of
      //an update() being one change of the state machine, do nothing and exit early
      rbusObject_Release(data);
      for (rbusValue_t value : {val,faultc,faultstr,resolved,startTime,completeTime}) { rbusValue_Release(value); }
      return;
   }
   else {
      //the field that changed is not status or URL
      //No handling for this yet so just set some basic values with an error code
      rbusValue_SetString(val,"Unknown");
      rbusValue_SetUInt16(faultc,9001);
      rbusValue_SetString(faultstr,"Unknown DU change");
      rbusValue_SetBoolean(resolved,false);
   }

   rbusObject_SetValue(data,"Resolved",resolved);
   rbusObject_SetValue(data,"StartTime",startTime);
   rbusObject_SetValue(data,"CompleteTime",completeTime);
   rbusObject_SetValue(data,"OperationPerformed",val);
   rbusObject_SetValue(data,"FaultCode",faultc);
   rbusObject_SetValue(data,"FaultString",faultstr);

   duevent.name = DEVICE_SM_PREFIX "DUStateChange!";
   duevent.type = RBUS_EVENT_GENERAL;
   duevent.data = data;

   rbusEvent_Publish(handle,&duevent);
   rbusObject_Release(data);
   for (rbusValue_t value : {val,faultc,faultstr,resolved,startTime,completeTime}) { rbusValue_Release(value); }
}


void dsm_rbus_provider::updateDU(rbusObject_t inParams, rbusMethodAsyncHandle_t asyncHandle,table_row & DU_row) {
   rbusError_t ec = rbusError_t::RBUS_ERROR_BUS_ERROR;
   rbusObject_t outParams;
   rbusObject_Init(&outParams,nullptr);

   //TODO DSM Update 


   //async handle passthrough
   rbusValue_t handle = rbusObject_GetValue(inParams,"uspasynchandle");
   if(handle!=nullptr) { rbusObject_SetValue(outParams,"uspasynchandle",handle); }

   rbusError_t sent = rbusMethod_SendAsyncResponse(asyncHandle,ec,outParams);
   if(sent != RBUS_ERROR_SUCCESS) { std::cerr << "Failed to return result from async method\n"; } 

   rbusObject_Release(inParams);
   rbusObject_Release(outParams);
}

rbusError_t rbus_table::tableMethodHandler (UNUSED_CHECK rbusHandle_t handle, UNUSED_CHECK char const* methodName,UNUSED_CHECK rbusObject_t inParams,UNUSED_CHECK rbusObject_t outParams, UNUSED_CHECK rbusMethodAsyncHandle_t asyncHandle) {

   std::string name (methodName);
   //truncate the prefix from the name, so that we end up with the table keys as the first element
   if (name.find(std::string(TABLE_ROOT)) != std::string::npos ) {
      name.erase(0,sizeof(TABLE_ROOT)-1);
   }

   std::vector<std::string> split =splitStringToVector(name);

   rbus_table * table =  findTable(split,true);
   if (table != nullptr) {
      size_t const index = getTableIndex(split[split.size()-2]);
      auto row_it = table->rows.find(index);
      if (row_it != table->rows.end()) {
         std::string const function_name = name.substr(name.find_last_of('.')+1);
         /*
            Compare function_name with the rbus method name to call the
            correct one here
         */
         if (function_name == "Uninstall()") {
            {
               rbusObject_Retain(inParams);
               rbusValue_t index_value;
               rbusValue_Init(&index_value);
               rbusValue_SetUInt32(index_value,index);
               rbusObject_SetValue(inParams,"index",index_value);
               dsm_rbus_provider::uninstall(inParams, asyncHandle, outParams);
               rbusObject_Release(inParams);
               return RBUS_ERROR_SUCCESS;
            }
         }
         else if (function_name == "SetRequestedState()") {
            return dsm_rbus_provider::SetRequestedState(row_it->second, inParams, outParams);
         }
         else if (function_name == "SetRunLevel()") {
            //TODO implement DSM SetRunLevel
            return (rbusError_t::RBUS_ERROR_BUS_ERROR);
         }
         else if (function_name == "Update()") {
            rbusObject_Retain(inParams);
            try {   
               //TODO async call needs mutex or similar protection   
               //TODO thread to call Update in DSM
                rbusObject_Release(inParams);
            }
            catch(...) {
               std::cerr<<"Failed to spawn thread\n";
               return RBUS_ERROR_BUS_ERROR;
            }
            return RBUS_ERROR_ASYNC_RESPONSE;
         }
      }
   } else {
      std::cerr << "table not found for method " << methodName << std::endl;
   }
   
   return (rbusError_t::RBUS_ERROR_BUS_ERROR);
}

//Updates an existing EE entry
void dsm_rbus_provider::update_ee_entry(int index, nlohmann::json &data) {
   uint32_t inst = dsm_rbus_provider::rbus_ee_instance_map[index];

   //only reassign if it has actually changed. could potentially fire false events otherwise
   //if the source of the change is on the rbus side then it will alreadt have changed
   if(tables["ExecEnv"].rows[inst]["Enable"].rbus_bool != data["enabled"])
      tables["ExecEnv"].rows[inst]["Enable"].rbus_bool = data["enabled"];

   if(tables["ExecEnv"].rows[inst]["Status"].rbus_string != data["status"])
      tables["ExecEnv"].rows[inst]["Status"].rbus_string = data["status"];

   if(tables["ExecEnv"].rows[inst]["Name"].rbus_string != data["name"])
      tables["ExecEnv"].rows[inst]["Name"].rbus_string = data["name"];

   if(tables["ExecEnv"].rows[inst]["InitialRunLevel"].rbus_int != data["InitialRunLevel"])
      tables["ExecEnv"].rows[inst]["InitialRunLevel"].rbus_int = data["InitialRunLevel"];

   if(tables["ExecEnv"].rows[inst]["CurrentRunLevel"].rbus_int != data["CurrentRunLevel"])
      tables["ExecEnv"].rows[inst]["CurrentRunLevel"].rbus_int = data["CurrentRunLevel"];
}

//Updates an existing DU entry
void dsm_rbus_provider::update_du_entry(std::string url, nlohmann::json &data) {
   
   auto iter = dsm_rbus_provider::rbus_du_instance_map.find(url);
   if (iter != dsm_rbus_provider::rbus_du_instance_map.end())
   {
      uint32_t inst = iter->second;

      if(tables["DeploymentUnit"].rows[inst]["URL"].rbus_string != data["URI"])
         tables["DeploymentUnit"].rows[inst]["URL"].rbus_string = data["URI"];

      if(tables["DeploymentUnit"].rows[inst]["Status"].rbus_string != data["state"])
         tables["DeploymentUnit"].rows[inst]["Status"].rbus_string = data["state"];

      std::string exec_value = "Device.SoftwareModules.ExecEnv." + std::to_string(inst);

      if(tables["DeploymentUnit"].rows[inst]["ExecutionEnvRef"].rbus_string != exec_value)
         tables["DeploymentUnit"].rows[inst]["ExecutionEnvRef"].rbus_string = exec_value;

      std::string exec_unit_list = "Device.SoftwareModules.ExecutionUnit." + std::to_string(inst);
      
      if(tables["DeploymentUnit"].rows[inst]["ExecutionUnitList"].rbus_string != exec_unit_list)
         tables["DeploymentUnit"].rows[inst]["ExecutionUnitList"].rbus_string = exec_unit_list;
   }

}
void dsm_rbus_provider::remove_du_entry(std::string url) 
{
    uint32_t inst = dsm_rbus_provider::rbus_du_instance_map[url];
    std::string path = std::string(DEVICE_SM_PREFIX"DeploymentUnit.") + std::to_string(inst);
    auto rc = rbusTable_removeRow(rbusHandle, path.c_str());  
    std::cout << "remove_du_entry rbusTable_removeRow path=" << path.c_str() << ", rc=" << (int)rc << std::endl;
    rbus_du_instance_map.erase(url);
}


//Updates an existing EU entry
void dsm_rbus_provider::update_eu_entry(std::string uid, nlohmann::json &data) {
   auto iter = dsm_rbus_provider::rbus_eu_instance_map.find(uid);
   if (iter != dsm_rbus_provider::rbus_eu_instance_map.end())
   {
      uint32_t inst = iter->second;

      if(tables["ExecutionUnit"].rows[inst]["Name"].rbus_string != data["uid"])
         tables["ExecutionUnit"].rows[inst]["Name"].rbus_string = data["uid"];


      if(tables["ExecutionUnit"].rows[inst]["Status"].rbus_string != data["status"])
         tables["ExecutionUnit"].rows[inst]["Status"].rbus_string = data["status"];
   }

}
void dsm_rbus_provider::remove_eu_entry(std::string uid) 
{
   uint32_t inst = dsm_rbus_provider::rbus_eu_instance_map[uid];
   std::string path = std::string(DEVICE_SM_PREFIX"ExecutionUnit.") + std::to_string(inst);
   auto rc = rbusTable_removeRow(rbusHandle, path.c_str());  
   std::cout << "remove_eu_entry rbusTable_removeRow path=" << path.c_str() << ", rc=" << (int)rc << std::endl;
   rbus_eu_instance_map.erase(uid);
}


// Takes a pointer to the parent to access non static fns
void dsm_rbus_provider::ee_worker_fn(dsm_rbus_provider *parent) {
   while (parent->get_worker_active()) {
      std::vector<nlohmann::json> ee_data;
      nlohmann::json list = dsm_rbus_provider::DSM_ref->ee_list(nullptr);

      for(std::string ee : list) {
         nlohmann::json ee_info = dsm_rbus_provider::DSM_ref                                 
                                    ->find_execution_environment(ee)
                                    ->to_json();
         ee_data.push_back(ee_info);
      }
      for(size_t i=0; i<ee_data.size(); i++) {
         if(ee_data[i] != parent->ee_cache[i]) {
            //update individual entries
            std::cout << "EE index " << i << ", name: " << ee_data[i]["name"] << " has changed!\n";
            parent->update_ee_entry(i,ee_data[i]);
            parent->ee_cache[i] = ee_data[i]; //cache update
         }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }
}

void dsm_rbus_provider::du_update_fn(dsm_rbus_provider *parent) 
{
   std::vector<nlohmann::json> du_data;
   nlohmann::json du_list = dsm_rbus_provider::DSM_ref->du_list(nullptr);
   //std::cout << "du_worker_fn du_list.size()=" << du_list.size() << std::endl;
   for(size_t i=0; i< du_list.size(); i++) 
   {
      auto json = du_list[i];
      std::string str = (std::string)json;
      std::shared_ptr<DeploymentUnit> deployment_unit = dsm_rbus_provider::DSM_ref->find_deployment_unit(str);
      if (deployment_unit != nullptr)
      {
         nlohmann::json du_info = deployment_unit->get_detail();
         du_data.push_back(du_info);
      }
   }
   for(size_t i=0; i<du_data.size(); i++) 
   {
      std::string url = du_data[i]["URI"];
      auto iter = parent->du_cache.find(url);
      if (iter != parent->du_cache.end())
      {
         if(du_data[i] != parent->du_cache[url]) {
            //update individual entries
            auto json_entry = du_data[i];
            std::cout << "DU index " << i << ", json_entry " << json_entry.dump().c_str() << " has changed!\n";
            parent->update_du_entry(url,du_data[i]);
            parent->du_cache[url] = du_data[i]; //cache update
         }
      }
      else
      {
         std::shared_ptr<DeploymentUnit> deployment_unit = dsm_rbus_provider::DSM_ref->find_deployment_unit(url);
         if (deployment_unit != nullptr)
         {
            nlohmann::json du_data = deployment_unit->get_detail();
            std::string url2 = du_data["URI"];
            if(parent->add_du_entry(url2,du_data)) 
            {
               parent->du_cache.insert({url2,du_data});
            }
         }
      }
   }
   if (du_list.size() != parent->du_cache.size())
   {
      std::vector<std::string> remove_list;
      for(auto &pair : parent->du_cache) 
      {
         std::shared_ptr<DeploymentUnit> deployment_unit = dsm_rbus_provider::DSM_ref->find_deployment_unit(pair.first);
         if (deployment_unit == nullptr)
         {
            parent->remove_du_entry(pair.first);
            remove_list.push_back(pair.first);
         }
      }
      for (auto& url : remove_list)
      {
         parent->du_cache.erase(url);
      }
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
void dsm_rbus_provider::eu_update_fn(dsm_rbus_provider *parent) 
{
   std::vector<nlohmann::json> eu_data_list;
   nlohmann::json eu_list = dsm_rbus_provider::DSM_ref->eu_list(nullptr);

   for(size_t i=0; i< eu_list.size(); i++) 
   {
      std::string str = (std::string)eu_list[i];
      ExecutionUnit* eu_data = dsm_rbus_provider::DSM_ref->find_execution_unit(str);
      if (eu_data != nullptr)
      {
         auto eu_data_detail = eu_data->get_detail();
         eu_data_list.push_back(eu_data_detail);
      }
   }
   for(size_t i=0; i<eu_data_list.size(); i++) {
      std::string uid = eu_data_list[i]["uid"];
      auto iter = parent->eu_cache.find(uid);
      if (iter != parent->eu_cache.end())
      {
         if(eu_data_list[i] != parent->eu_cache[uid]) {
            //update individual entries
            std::cout << "EU index " << i << ", name: " << eu_data_list[i]["ee"] << " has changed!\n";
            parent->update_eu_entry(uid,eu_data_list[i]);
            parent->eu_cache[uid] = eu_data_list[i];
         }
      }
      else
      {
         std::string str = (std::string)eu_list[i];
         ExecutionUnit* eu_data = dsm_rbus_provider::DSM_ref->find_execution_unit(str);
         if (eu_data != nullptr)
         {
            auto eu_data_detail = eu_data->get_detail();
            std::string uid = eu_data_detail["uid"];
            if(parent->add_eu_entry(uid,eu_data_detail)) 
            {
               parent->eu_cache.insert({uid,eu_data_detail});
            }
         }
      }
   }
   if (eu_list.size() != parent->eu_cache.size())
   {
      std::vector<std::string> remove_list;
      for(auto &pair : parent->eu_cache) 
      {
         ExecutionUnit* eu_data = dsm_rbus_provider::DSM_ref->find_execution_unit(pair.first);
         if (eu_data == nullptr)
         {
            parent->remove_eu_entry(pair.first);
            remove_list.push_back(pair.first);
         }
      }
      for (auto& uid : remove_list)
      {
         parent->eu_cache.erase(uid);
      }
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
// Takes a pointer to the parent to access non static fns
void dsm_rbus_provider::du_eu_worker_fn(dsm_rbus_provider *parent) {
   while (parent->get_worker_active()) 
   {
      du_update_fn(parent);
      eu_update_fn(parent);
   }
}


// Helper function that tries to create an entry in the EE rbus table from the json output of ExecutionEnvironment::to_json()
bool dsm_rbus_provider::add_ee_entry(int index, nlohmann::json &data) {
   rbusError_t rc;
   uint32_t inst;

   rc = rbusTable_addRow(rbusHandle,DEVICE_SM_PREFIX "ExecEnv.",nullptr,&inst);
   if(rc == RBUS_ERROR_SUCCESS) {
      tables["ExecEnv"].rows[inst]["Enable"].current_type = rbusValueType_t::RBUS_BOOLEAN;
      tables["ExecEnv"].rows[inst]["Enable"].rbus_bool = data["enabled"];

      tables["ExecEnv"].rows[inst]["Status"].current_type = rbusValueType_t::RBUS_STRING;
      tables["ExecEnv"].rows[inst]["Status"].rbus_string = data["status"];

      tables["ExecEnv"].rows[inst]["Name"].current_type = rbusValueType_t::RBUS_STRING;
      tables["ExecEnv"].rows[inst]["Name"].rbus_string = data["name"];

      tables["ExecEnv"].rows[inst]["InitialRunLevel"].current_type = rbusValueType_t::RBUS_INT32;
      tables["ExecEnv"].rows[inst]["InitialRunLevel"].rbus_int = data["InitialRunLevel"];

      tables["ExecEnv"].rows[inst]["CurrentRunLevel"].current_type = rbusValueType_t::RBUS_INT32;
      tables["ExecEnv"].rows[inst]["CurrentRunLevel"].rbus_int = data["CurrentRunLevel"];

      rbus_ee_instance_map.insert({index,inst});
      return true;
   }

   return false;
}

// Helper function that tries to create an entry in the DU rbus table from the json output of ExecutionEnvironment::to_json()
bool dsm_rbus_provider::add_du_entry(std::string url, nlohmann::json &data) 
{
   rbusError_t rc;
   uint32_t inst;

   rc = rbusTable_addRow(rbusHandle,DEVICE_SM_PREFIX "DeploymentUnit.",nullptr,&inst);
   std::cout << "add_du_entry rbusTable_addRow rc=" << (int)rc << std::endl; 
   if(rc == RBUS_ERROR_SUCCESS) {
      
      tables["DeploymentUnit"].rows[inst]["URL"].current_type = rbusValueType_t::RBUS_STRING;
      tables["DeploymentUnit"].rows[inst]["URL"].rbus_string = data["URI"];

      tables["DeploymentUnit"].rows[inst]["Status"].current_type = rbusValueType_t::RBUS_STRING;
      tables["DeploymentUnit"].rows[inst]["Status"].rbus_string = data["state"];

      std::string exec_value = "Device.SoftwareModules.ExecEnv." + std::to_string(inst);

      tables["DeploymentUnit"].rows[inst]["ExecutionEnvRef"].current_type = rbusValueType_t::RBUS_STRING;
      tables["DeploymentUnit"].rows[inst]["ExecutionEnvRef"].rbus_string = exec_value;

      std::string exec_unit_list = "Device.SoftwareModules.ExecutionUnit." + std::to_string(inst);

      tables["DeploymentUnit"].rows[inst]["ExecutionUnitList"].current_type = rbusValueType_t::RBUS_STRING;
      tables["DeploymentUnit"].rows[inst]["ExecutionUnitList"].rbus_string = exec_unit_list;


      rbus_du_instance_map.insert({url,inst});
      return true;
   }

   return false;
}
// Helper function that tries to create an entry in the DU rbus table from the json output of ExecutionEnvironment::to_json()
bool dsm_rbus_provider::add_eu_entry(std::string uid, nlohmann::json &data) {
   rbusError_t rc;
   uint32_t inst;

   rc = rbusTable_addRow(rbusHandle,DEVICE_SM_PREFIX "ExecutionUnit.",nullptr,&inst);
   std::cout << "add_eu_entry rbusTable_addRow rc=" << (int)rc << std::endl;
   if(rc == RBUS_ERROR_SUCCESS) {
      
      tables["ExecutionUnit"].rows[inst]["Name"].current_type = rbusValueType_t::RBUS_STRING;
      tables["ExecutionUnit"].rows[inst]["Name"].rbus_string = data["uid"];

      auto state = data["status"];

      tables["ExecutionUnit"].rows[inst]["Status"].current_type = rbusValueType_t::RBUS_STRING;
      tables["ExecutionUnit"].rows[inst]["Status"].rbus_string = data["status"];


      rbus_eu_instance_map.insert({uid,inst});
      return true;
   }

   return false;
}

void dsm_rbus_provider::ee_sync_table_init() {
   
   nlohmann::json list = dsm_rbus_provider::DSM_ref->ee_list(nullptr);
   std::vector<nlohmann::json> ee_data;
   for(std::string ee : list) 
   {
      auto ee_obj = dsm_rbus_provider::DSM_ref->find_execution_environment(ee);
      nlohmann::json ee_info = ee_obj->to_json();
      ee_data.push_back(ee_info);
   }
   for(size_t i=0; i<ee_data.size(); i++) 
   {
      if(add_ee_entry(i,ee_data[i])) 
      {
         ee_cache.insert({i,ee_data[i]});
      }
   }
}
void dsm_rbus_provider::du_sync_table_init() {
   nlohmann::json dus = dsm_rbus_provider::DSM_ref->du_list(nullptr);
   for(size_t i=0; i< dus.size(); i++) {
      auto json = dus[i];
      std::string str = (std::string)json;
      std::shared_ptr<DeploymentUnit> deployment_unit = dsm_rbus_provider::DSM_ref->find_deployment_unit(str);
      if (deployment_unit != nullptr)
      {
         nlohmann::json du_data = deployment_unit->get_detail();
         std::string url = du_data["URI"];
         if(add_du_entry(url,du_data)) {
               du_cache.insert({url,du_data});
         }
      }
   }
}

void dsm_rbus_provider::eu_sync_table_init() {
   nlohmann::json eu_list = dsm_rbus_provider::DSM_ref->eu_list(nullptr);
   for(size_t i=0; i< eu_list.size(); i++) {
      std::string str = (std::string)eu_list[i];
      ExecutionUnit* eu_data = dsm_rbus_provider::DSM_ref->find_execution_unit(str);
      if (eu_data != nullptr)
      {
         auto eu_data_detail = eu_data->get_detail();
         std::string uid = eu_data_detail["uid"];
         if(add_eu_entry(uid,eu_data_detail)) {
               eu_cache.insert({uid,eu_data_detail});
         }
      }
   }
}


/*
* event handler for the ee_external_change event. Makes the relevant changes in dsm when a change is made in rbus
*/
void dsm_rbus_provider::ee_external_change_handler(UNUSED_CHECK rbusHandle_t handle, rbusEvent_t const* event,UNUSED_CHECK rbusEventSubscription_t* subscription) 
{
   std::cout << "dsm_rbus_provider::ee_external_change_handler" << std::endl;

   std::string field(rbusValue_GetString(rbusObject_GetValue(event->data,"field"),nullptr));
   uint32_t inst = rbusValue_GetUInt32(rbusObject_GetValue(event->data,"inst"));
   rbusValue_t val = rbusObject_GetValue(event->data,"value");

   //small number of instances so linear lookup should be fine here
   int index = -1;
   for(auto &pair : dsm_rbus_provider::rbus_ee_instance_map) 
   {
      std::cout << "dsm_rbus_provider::ee_external_change_handler pair.second=" << pair.second << std::endl;
      if(pair.second==inst) {
         index = pair.first;
         break;
      }
   }

   // for EE the only writeable fields are InitialRunlevel and Enable, int and bool respectively
   // type checking has already been done so it is safe to assume thats what we'll get
   if(index>=0) {
      auto ee =  DSM_ref->find_execution_environment((std::string) DSM_ref->ee_list(nullptr)[index]);
      if (ee != nullptr)
      {
         if (field == "Enable") {
            ee->set_enable(rbusValue_GetBoolean(val));
         }
         else if(field == "InitialRunLevel") {
            ee->set_initial_runlevel((int) rbusValue_GetInt32(val));
         }
      }
   }
}

/*
* event handler for the du_external_change event. Makes the relevant changes in dsm when a change is made in rbus
*/
void dsm_rbus_provider::du_external_change_handler(UNUSED_CHECK rbusHandle_t handle, rbusEvent_t const* event,UNUSED_CHECK rbusEventSubscription_t* subscription) 
{
   std::cout << "dsm_rbus_provider::du_external_change_handler" << std::endl;
}
/*
* event handler for the eu_external_change event. Makes the relevant changes in dsm when a change is made in rbus
*/
void dsm_rbus_provider::eu_external_change_handler(UNUSED_CHECK rbusHandle_t handle, rbusEvent_t const* event,UNUSED_CHECK rbusEventSubscription_t* subscription) 
{
   std::cout << "dsm_rbus_provider::eu_external_change_handler" << std::endl;
}
/*
* custom set hander for writeable table elements. Fires an event for mirroring then calls the generic handler.
* Depending on the table it will fire: ee_external_change, du_external_change, or eu_external_change.
* As of writing only ee_external_change has a handler.
*/
rbusError_t dsm_rbus_provider::dsmTableSetHandler(rbusHandle_t handle, rbusProperty_t property,rbusSetHandlerOptions_t* options) {
   rbusEvent_t ev {};
   rbusObject_t data;
   std::string path(rbusProperty_GetName(property)), field, table;
   uint32_t inst;

   rbusObject_Init(&data,nullptr);

   auto split = utility_functions::splitStringToVector(path);
   field = split.back();
   split.pop_back();
   inst = (uint32_t) std::stoul(split.back());
   split.pop_back();
   table = split.back();

   rbusValue_t val = rbusProperty_GetValue(property);

   rbusValue_t fval;
   rbusValue_Init(&fval);
   rbusValue_SetString(fval,field.c_str());
   rbusObject_SetValue(data,"field",fval);

   rbusValue_t instval;
   rbusValue_Init(&instval);
   rbusValue_SetUInt32(instval,inst);
   rbusObject_SetValue(data,"inst",instval);

   rbusObject_SetValue(data,"value",val);

   const char* event_name;
   if(table=="ExecEnv") {
      event_name = "ee_external_change";
   }
   else if(table=="DeploymentUnit") {
      event_name = "du_external_change";
   }
   else {
      event_name = "eu_external_change";
   }

   std::cout << "dsm_rbus_provider::dsmTableSetHandler( event_name=" << event_name << std::endl;

   ev.name = event_name;
   ev.type = RBUS_EVENT_GENERAL;
   ev.data = data;

   //add versions of this for du and eu as needed
   static std::map<std::string,rbusValueType_t> ee_value_types = {{"Enable",RBUS_BOOLEAN},{"InitialRunLevel",RBUS_INT32}};

   static std::map<std::string,rbusValueType_t> du_value_types = {};

   if(table=="ExecEnv") {
      if(ee_value_types[field] == rbusValue_GetType(val)) {
         rbusEvent_Publish(rbusHandle,&ev);

         rbusValue_Release(instval);
         rbusValue_Release(fval);
         rbusObject_Release(data);
         return rbus_table::tableSetHandler(handle,property,options);
      }
   }
   else if(table=="DeploymentUnit"){
   if(du_value_types[field] == rbusValue_GetType(val)) {
         rbusEvent_Publish(rbusHandle,&ev);

         rbusValue_Release(instval);
         rbusValue_Release(fval);
         rbusObject_Release(data);
         return rbus_table::tableSetHandler(handle,property,options);
      }
   }
   else if(table=="ExecutionUnit") {
      if(du_value_types[field] == rbusValue_GetType(val)) {
         rbusEvent_Publish(rbusHandle,&ev);

         rbusValue_Release(instval);
         rbusValue_Release(fval);
         rbusObject_Release(data);
         return rbus_table::tableSetHandler(handle,property,options);
      }
   }
      
   rbusValue_Release(instval);
   rbusValue_Release(fval);
   rbusObject_Release(data);
   return rbusError_t::RBUS_ERROR_INVALID_INPUT;
}
