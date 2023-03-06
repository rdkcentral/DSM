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

#include "rbus_macros.h"
#include "provider.h"
#include <vector>

#ifndef _DEVICESM_RBUS_TABLE_H
#define _DEVICESM_RBUS_TABLE_H

//the root path for the table.
#define TABLE_ROOT DEVICE_SM_PREFIX

using table_row = std::map<std::string,struct rbus_data>;
using rbus_map = std::map<std::string,struct rbus_table>;

class rbus_table {
public:
    rbus_table (std::string const & in_name,table_row const& row_definiton);
    rbus_table() = default; 

    inline table_row const& GetTemplateRow() { return template_row; }
    void open(rbusHandle_t rbusHandle);
    void addSubTable(rbus_table sub_table);
    void prefixSubTableNames(std::string const & prefix,rbus_map& map);
    std::string const& getName() { return table_name; }
    //rows and sub tables are public (for now) as they may need to get got/set
    std::map<size_t,table_row> rows;
    std::map<size_t,rbus_map> sub_tables;
    rbus_map template_sub_table; //be nice to make this private, but it needs to be changed when adding subtables to append prefixes
private:
    std::string const table_name;
    std::string long_table_name;
    std::map<std::string,std::string> row_names;
    table_row template_row;
    
    
    //find functions return nullptr if not found
    UNUSED_RESULT_CHECK static rbus_data  * find(rbusProperty_t property);
    UNUSED_RESULT_CHECK static rbus_table * findTable(rbusProperty_t property,bool contains_value); //set bool to true if the string contains the value as well as the tables
    UNUSED_RESULT_CHECK static rbus_table * findTable(std::string const & name,bool contains_value);
    UNUSED_RESULT_CHECK static rbus_table * findTable(std::vector<std::string> const & split,bool contains_value);
    void setElements(std::vector<rbusDataElement_t>& elements);
    UNUSED_RESULT_CHECK static size_t getTableIndex(std::string const& in);

//getters/setters need to be public otherwise they can't be used as function pointers
public:
    UNUSED_RESULT_CHECK static rbusError_t addRow(rbusHandle_t handle,char const* tableName,char const* aliasName, uint32_t* instNum);
    UNUSED_RESULT_CHECK static rbusError_t removeRow(rbusHandle_t handle, char const* rowName);
    UNUSED_RESULT_CHECK static rbusError_t tableGetHandler (rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options);
    UNUSED_RESULT_CHECK static rbusError_t tableSetHandler (rbusHandle_t handle, rbusProperty_t property,rbusSetHandlerOptions_t* options);
    UNUSED_RESULT_CHECK static rbusError_t tableEventHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish);
    UNUSED_RESULT_CHECK static rbusError_t tableMethodHandler (rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);

};

#endif