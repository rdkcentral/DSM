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

#ifndef DATA_MODEL_PROVIDER_H
#define DATA_MODEL_PROVIDER_H

#include <rbus.h>
#include <map>
#include <cstdint>
#include <thread>
#include "rbus_macros.h"
#include "table.h"

struct rbus_data {
    explicit rbus_data(rbusDataElement_t const& element) : internal_element(element) {}
    rbus_data() = default;
    ~rbus_data() = default;
    
    rbusDataElement_t internal_element;
    rbusValueType_t current_type  = RBUS_NONE;
    union{ 
        bool rbus_bool;
        uint8_t rbus_byte;
        uint16_t rbus_short;
        uint32_t rbus_int;
        uint64_t rbus_long_int = 0;
        float    rbus_float;
        double   rbus_double;
        rbusDateTime_t rbus_time;
    }; 
    std::string rbus_string {};
    //not implemented yet
    //date_time
    //property
    //object
    //bytes
};


class rbus_provider {
public:
    rbus_provider(const char* name);
    virtual ~rbus_provider();

    static std::map<std::string,rbus_table> tables;

    UNUSED_RESULT_CHECK static rbusError_t getHandler (rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options);
    UNUSED_RESULT_CHECK static rbusError_t setHandler (rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options);
    UNUSED_RESULT_CHECK static rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName,rbusFilter_t filter,int32_t interval,bool* autoPublish);
    UNUSED_RESULT_CHECK static rbusError_t setData(rbus_data& data,rbusProperty_t property);
    static void getData(rbus_data& data,rbusProperty_t property);

protected:
    #define DEVICE_SM_PREFIX "Device.SoftwareModules."
    static std::map< std::string, rbus_data > elements;
    static rbusHandle_t rbusHandle;

    void registerPath(std::string path,rbus_data value);
    void registerTables();
    
private:
    const char* provider_name;
};

namespace rbus_callback_tables{
    //generic callback tables                                   //get                       //set                       //add row           //remove row            //event subscribe               //method
    constexpr rbusCallbackTable_t rbus_generic_read_write       {rbus_provider::getHandler  ,rbus_provider::setHandler  ,nullptr            ,nullptr                ,nullptr                        ,nullptr};
    constexpr rbusCallbackTable_t rbus_generic_read_only        {rbus_provider::getHandler  ,nullptr                    ,nullptr            ,nullptr                ,nullptr                        ,nullptr};
    constexpr rbusCallbackTable_t rbus_table_rows               {nullptr                    ,nullptr                    ,rbus_table::addRow ,rbus_table::removeRow  ,rbus_table::tableEventHandler  ,nullptr};
    constexpr rbusCallbackTable_t rbus_table_generic_read_write {rbus_table::tableGetHandler,rbus_table::tableSetHandler,nullptr            ,nullptr                ,nullptr                        ,nullptr};
    constexpr rbusCallbackTable_t rbus_table_generic_read_only  {rbus_table::tableGetHandler,nullptr                    ,nullptr            ,nullptr                ,nullptr                        ,nullptr};
    //constexpr rbusCallbackTable_t rbus_generic_method           {nullptr                    ,nullptr                    ,nullptr            ,nullptr                ,nullptr                        ,rbus_provider::methodHandler};
    constexpr rbusCallbackTable_t rbus_table_generic_method     {nullptr                    ,nullptr                    ,nullptr            ,nullptr                ,nullptr                        ,rbus_table::tableMethodHandler};
    constexpr rbusCallbackTable_t rbus_event                    {nullptr                    ,nullptr                    ,nullptr            ,nullptr                ,rbus_provider::eventSubHandler,nullptr};
}
#endif