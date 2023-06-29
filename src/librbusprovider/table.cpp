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

#include "table.h"
#include "utility.h"
#include <string.h>
#include <iostream>
#include <sstream>

using namespace rbus_callback_tables;
using namespace utility_functions;

std::map<std::string,rbus_table> rbus_provider::tables;

rbus_table::rbus_table(std::string const & in_name,table_row const& row_definiton) : 
table_name (in_name),
long_table_name (std::string(TABLE_ROOT ) + in_name + ".{i}."),
template_row(row_definiton) {
}

//alias name is the table key. it can be anything but we want it to be an index starting at 1 and incrementing up to match what rbus is doing internally
//ignore aliasName and use index somehow
rbusError_t rbus_table::addRow(UNUSED_CHECK rbusHandle_t handle,char const* tableName,UNUSED_CHECK char const* aliasName,UNUSED_CHECK uint32_t* instNum){ 
    std::string name (tableName);
    
    //truncate the prefix from the name, so that we end up with the table keys as the first element
    if (name.find(std::string(TABLE_ROOT)) != std::string::npos ) {
        name.erase(0,sizeof(TABLE_ROOT)-1);
        //there should/could be a trailing '.', so remove it if it's there
        if (!name.empty() && name[name.length()-1] == '.') { name.erase(name.length()-1,1); }
    }

    rbus_table * table = findTable(name,false);
    if (table != nullptr) {
        size_t index = 0 ;
        if (aliasName == nullptr) {
            for (auto& row : table->rows)  {
                index = std::max(index,row.first);
            }
            index++;
        } else {
            index = getTableIndex(std::string(aliasName));
        }
        if (index != 0 ) {
            std::cout << "adding row "<< index <<" to table : " << table->long_table_name << std::endl;
            table->rows[index] = table->GetTemplateRow();
//use cast on 64 bit, cast may cause warnings due to being usless on 32bit
#if INTPTR_MAX == INT64_MAX
            *instNum =  static_cast<uint32_t>(table->rows.size());
#elif INTPTR_MAX == INT32_MAX
            *instNum =  table->rows.size();
#endif

            //add any subtables for this table
            for (auto i : table->template_sub_table){
                table->sub_tables[index].insert({i.second.table_name,i.second});
            }
            return (rbusError_t::RBUS_ERROR_SUCCESS);
        }
    }
    return (rbusError_t::RBUS_ERROR_BUS_ERROR);
}

//row name is the full rbus name, including the table name and the alias name
rbusError_t rbus_table::removeRow(UNUSED_CHECK rbusHandle_t handle,UNUSED_CHECK char const* rowName) {
    
    std::string name (rowName);
    
    //truncate the prefix from the name, so that we end up with the table keys as the first element
    if (name.find(std::string(TABLE_ROOT)) != std::string::npos ) {
        name.erase(0,sizeof(TABLE_ROOT)-1);
    }
    std::vector<std::string> split = splitStringToVector(name);
    size_t last_index = getTableIndex(split[split.size()-1]);

    for (auto & i : split) {
        std::cout << i << std::endl;
    }
    //findTable doesn't play nice with < 3 elements, so just do a regular find on the map
    rbus_table * table = nullptr;
    if (split.size() == 2) { //top level table
        auto table_it = rbus_provider::tables.find(split[0]);
        if (table_it !=  rbus_provider::tables.end()) {
            table = &table_it->second;
        }
    } else {
        table = findTable(name,false); //find the table but chop off the last index as the findTable function doesn't expect that
    }

    if (table != nullptr) {
        auto it = table->rows.find(last_index);
        if (it != table->rows.end()) {
            table->rows.erase(last_index);
            std:: cout << "removed row from " << name << " at index" << last_index << std::endl ;
            return (rbusError_t::RBUS_ERROR_SUCCESS);
        } else {
            std::cerr << "index " << last_index << "is out of bounds for table" << table->long_table_name << std::endl;
        }
    }
    return (rbusError_t::RBUS_ERROR_BUS_ERROR);
}

void rbus_table::setElements(std::vector<rbusDataElement_t>& elements) {
    elements.emplace_back(rbusDataElement_t {const_cast<char*>(long_table_name.c_str()), rbusElementType_t::RBUS_ELEMENT_TYPE_TABLE, rbus_table_rows});
    for (auto& i : template_row) {
        auto& row_element = i.second;
        row_names[i.first] = long_table_name + i.first;
        row_element.internal_element.name = const_cast<char*>(row_names[i.first].c_str());
        elements.emplace_back(row_element.internal_element);
    }
    for (auto& i :template_sub_table ) {
        i.second.setElements(elements);
    }
}

void rbus_table::open(rbusHandle_t rbusHandle){
    //table elements have to be registered at the same time, so this gathers them, and any nested tables set in this table, adds them all to the list and registers them in one go
    //the ordering is also important i.e. you need to add the table before any table element
    std::vector<rbusDataElement_t> elements;
    setElements(elements);
    for (auto i : elements) {
        std::cout << i.name << std::endl;
    }
    rbusError_t error_code = rbus_regDataElements(rbusHandle,static_cast<int>(elements.size()),&elements[0]);
    if (error_code != RBUS_ERROR_SUCCESS) { 
            std::cerr << " one or more of the table elements in table in " << table_name << " returned error=" << rbusError_ToString(error_code) << std::endl;
            rbus_close(rbusHandle);
            throw; 
    }
}

rbus_data * rbus_table::find(rbusProperty_t property) {
    std::string name = std::string(rbusProperty_GetName(property)); 

    //truncate the prefix from the name, so that we end up with the table keys as the first element
    if (name.find(std::string(TABLE_ROOT)) != std::string::npos ) {
        name.erase(0,sizeof(TABLE_ROOT)-1);
    }

    std::vector<std::string> split =splitStringToVector(name);
    if (split.size() < 3) {
        std::cerr << "vector too small" << std::endl;
        return (nullptr);
    }

    rbus_table * table = findTable(property,true);
    if (table != nullptr) {
        std::string const & sub_table_index = split[split.size()-2];
        std::string const & sub_table_value = split[split.size()-1];
        size_t index = getTableIndex(sub_table_index); //set index to sub-table index

        auto table_it = table->rows.find(index);
        if (table_it == table->rows.end()) {
            std::cerr << "row index of "<< index << " out of bounds for table " << table->getName() << std::endl;
            return (nullptr);
        }

        auto row_it = table->rows[index].find(sub_table_value); //get the row of the subtable

        if (row_it != table->rows[index].end()) {
            return (&row_it->second);
        }
    }
    std::cerr << "could not find property " << name << std::endl;
    return nullptr;
}

rbus_table * rbus_table::findTable(rbusProperty_t property,bool contains_value = false) {
    std::string name = std::string(rbusProperty_GetName(property)); 

    //truncate the prefix from the name, so that we end up with the table keys as the first element
    if (name.find(std::string(TABLE_ROOT)) != std::string::npos ) {
        name.erase(0,sizeof(TABLE_ROOT)-1);
    }
    return findTable(name,contains_value);
}

rbus_table * rbus_table::findTable(std::string const & name,bool contains_value = false) {
    std::vector<std::string> split =splitStringToVector(name);
    return findTable(split,contains_value);
}

rbus_table * rbus_table::findTable(std::vector<std::string> const & split,bool contains_value = false) {  

    auto table_it = rbus_provider::tables.find(split[0]);

    if (table_it != rbus_provider::tables.end() ) {
        if (split.size() <= 3 - (contains_value ? 0 : 2)) {
            return (&table_it->second);
        } else { //indicates that a sub-table must be present

            size_t offset = 1;
            constexpr size_t next_index = 2;
            rbus_map::iterator sub_tables_cur;
            size_t row_index = 0;
            while (offset < split.size()-(contains_value ? 2 : 0)) {
                row_index = getTableIndex(split[offset]);
                sub_tables_cur = table_it->second.sub_tables[row_index].find(split[offset+1]);
                if (sub_tables_cur !=  table_it->second.sub_tables[row_index].end()) {
                    table_it = sub_tables_cur;
                    offset += next_index;
                } else {
                    std::cerr << "table " << split[offset+1] << " not found" << std::endl;
                    break;
                }
            }

            if (sub_tables_cur !=  table_it->second.sub_tables[row_index].end()) {
                return(&sub_tables_cur->second);
            }
        }
    }

    std::cerr << "table with key ";
    for (auto const & i : split) {
        std::cerr << i << "." ;
    }
    std::cerr   << " is not found" << std::endl;

    return nullptr;
}

rbusError_t rbus_table::tableGetHandler (UNUSED_CHECK rbusHandle_t handle, rbusProperty_t property, UNUSED_CHECK rbusGetHandlerOptions_t* options) { 
    
    rbus_data * data = find(property);
    if (data != nullptr) {
        rbus_provider::getData(*data,property);
        return (rbusError_t::RBUS_ERROR_SUCCESS);
    }
    return (rbusError_t::RBUS_ERROR_BUS_ERROR);
}

rbusError_t rbus_table::tableSetHandler (UNUSED_CHECK rbusHandle_t handle, rbusProperty_t property,UNUSED_CHECK rbusSetHandlerOptions_t* options) {
    rbus_data * data = find(property);
    if (data != nullptr) {
        return (rbus_provider::setData(*data,property));
    }
    return (rbusError_t::RBUS_ERROR_BUS_ERROR);
}

rbusError_t rbus_table::tableEventHandler(UNUSED_CHECK rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName,UNUSED_CHECK  rbusFilter_t filter,UNUSED_CHECK  int32_t interval,UNUSED_CHECK  bool* autoPublish) {
    std::cout << "Event for table "<< eventName <<" called: action=" << (action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe") << std::endl;
    return RBUS_ERROR_SUCCESS;
}

void rbus_table::prefixSubTableNames(std::string const & prefix,rbus_map& map) { //TODO improve this function , i don't like it
    for (auto& i : map) {
        i.second.long_table_name = prefix + i.second.table_name + ".{i}.";
        prefixSubTableNames(prefix,i.second.template_sub_table);
    }
}

void rbus_table::addSubTable(rbus_table sub_table) {
    std::string key = sub_table.table_name ;
    sub_table.long_table_name = long_table_name + sub_table.table_name + ".{i}.";
    prefixSubTableNames(sub_table.long_table_name,sub_table.template_sub_table); //fix the long names recursively to have the correct prefix
    template_sub_table.insert({key,sub_table});
}

size_t rbus_table::getTableIndex(std::string const& in) {
    errno = 0;
    size_t ret_val = std::strtoul(in.c_str(),nullptr,10);
    if (errno != 0 || ret_val == 0) { //ret_val should never be 0 here
        std::cerr << "strtol returned: " << ret_val << " when parsing string\""<< in <<"\" errno=" << errno << std::endl;
    }
    return (ret_val);
}
