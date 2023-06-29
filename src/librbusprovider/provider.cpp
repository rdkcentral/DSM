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

#include "provider.h"
#include <iostream>

rbusHandle_t rbus_provider::rbusHandle {};

std::map< std::string,rbus_data > rbus_provider::elements = {};

rbus_provider::rbus_provider(const char* name)
    :provider_name(name)
{
    rbusError_t ec = rbus_open(&rbusHandle,provider_name);
    if(ec!=RBUS_ERROR_SUCCESS)
    {
        std::cerr << "Could not open RBUS\n" << "Returned error=" << rbusError_ToString(ec) << std::endl;
        throw;
    }
}

rbus_provider::~rbus_provider()
{
    for (auto& i : elements) {
        rbus_unregDataElements(rbusHandle,1,&i.second.internal_element);
    }
    rbus_close(rbusHandle);
}

void rbus_provider::getData(rbus_data& data,rbusProperty_t property) 
{
    rbusValue_t value = nullptr;
    rbusValue_Init(&value);
    switch(data.current_type){
        case RBUS_STRING : {
            rbusValue_SetString(value, data.rbus_string.c_str());
            break;
        }
		case RBUS_BYTE: {
            rbusValue_SetByte(value,data.rbus_byte);
            break;
        }
		case RBUS_INT8: {
            rbusValue_SetInt8(value,data.rbus_byte);
            break;
        }
		case RBUS_UINT8: {
            rbusValue_SetUInt8(value,data.rbus_byte);
            break;
        }
		case RBUS_INT16: {
            rbusValue_SetInt16(value,data.rbus_short);
            break;
        }
		case RBUS_UINT16: {
            rbusValue_SetUInt16(value,data.rbus_short);
            break;
        }
		case RBUS_INT32: {
            rbusValue_SetInt32(value,data.rbus_int);
            break;
        }
		case RBUS_UINT32: {
            rbusValue_SetUInt32(value,data.rbus_int);
            break;
        }
		case RBUS_INT64:  {
            rbusValue_SetInt64(value,data.rbus_long_int);
            break;
        }
		case RBUS_UINT64:  {
            rbusValue_SetUInt64(value,data.rbus_long_int);
            break;
        }
		case RBUS_SINGLE:  {
            rbusValue_SetSingle(value,data.rbus_float);
            break;
        }
		case RBUS_DOUBLE:  {
            rbusValue_SetDouble(value,data.rbus_double);
            break;
        }
		case RBUS_DATETIME: {
            rbusValue_SetTime(value,&data.rbus_time);
            break;
        }
        case RBUS_BOOLEAN:  {
            rbusValue_SetBoolean(value,data.rbus_bool);
            break;
        }
		case RBUS_BYTES:
		case RBUS_PROPERTY:
		case RBUS_OBJECT:
		case RBUS_NONE:
        default: {
            std::cerr << "not supported type value:" <<  data.current_type << std::endl;
            break;
        }
    }
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
}

rbusError_t rbus_provider::setData(rbus_data& data,rbusProperty_t property) 
{
    rbusValue_t value = rbusProperty_GetValue(property);
    rbusValueType_t const new_type = rbusValue_GetType(value);
    switch(new_type){
        case RBUS_STRING : {
            int len = 0;
            const char * str = rbusValue_GetString(value,&len);   
            if (str != nullptr) {
                data.rbus_string = std::string(str,len);
            } else {
                return rbusError_t::RBUS_ERROR_BUS_ERROR;
            }
            break;
        }
		case RBUS_BYTE: {
            data.rbus_byte = rbusValue_GetByte(value);
            break;
        }
		case RBUS_INT8: {
            data.rbus_byte = rbusValue_GetInt8(value);
            break;
        }
		case RBUS_UINT8: {
            data.rbus_byte = rbusValue_GetUInt8(value);
            break;
        }
		case RBUS_INT16: {
            data.rbus_short = rbusValue_GetInt16(value);
            break;
        }
		case RBUS_UINT16: {
            data.rbus_short = rbusValue_GetUInt16(value);
            break;
        }
		case RBUS_INT32: {
            data.rbus_int = rbusValue_GetInt32(value);
            break;
        }
		case RBUS_UINT32: {
            data.rbus_int = rbusValue_GetUInt32(value);
            break;
        }
		case RBUS_INT64:  {
            data.rbus_long_int = rbusValue_GetInt64(value);
            break;
        }
		case RBUS_UINT64:  {
            data.rbus_long_int = rbusValue_GetUInt64(value);
            break;
        }
		case RBUS_SINGLE:  {
            data.rbus_float = rbusValue_GetSingle(value);
            break;
        }
		case RBUS_DOUBLE:  {
            data.rbus_double = rbusValue_GetDouble(value);
            break;
        }
		case RBUS_DATETIME:{
            const rbusDateTime_t * tmp = rbusValue_GetTime(value);
            if (tmp != nullptr) {
                data.rbus_time = *tmp;
            } else {
                std::cerr << "Invalid time contained in value" << std::endl; 
            }
            break;
        }
        case RBUS_BOOLEAN:  {
            data.rbus_bool = rbusValue_GetBoolean(value);
            break;
        }
		case RBUS_BYTES:
		case RBUS_PROPERTY:
		case RBUS_OBJECT:
        default: {
            std::cerr << "not supported type value:" <<  data.current_type << std::endl;
            return (rbusError_t::RBUS_ERROR_BUS_ERROR);
        }
        case RBUS_NONE: //do nothing
            break;
    }
    data.current_type = new_type;
    return (rbusError_t::RBUS_ERROR_SUCCESS);
}

rbusError_t rbus_provider::setHandler (UNUSED_CHECK rbusHandle_t handle, UNUSED_CHECK rbusProperty_t property, UNUSED_CHECK rbusSetHandlerOptions_t* options) {
    
    char const* const name = rbusProperty_GetName(property); 
    if (name != nullptr) {
        auto const it = rbus_provider::elements.find(name);
        if (it != rbus_provider::elements.end() ) {
            return setData(it->second,property);
        }
    }     
    return rbusError_t::RBUS_ERROR_BUS_ERROR;
}

rbusError_t rbus_provider::getHandler (UNUSED_CHECK rbusHandle_t handle, rbusProperty_t property, UNUSED_CHECK rbusGetHandlerOptions_t* options) { 
    char const* const name = rbusProperty_GetName(property); 
    if (name != nullptr) {
        auto const it = rbus_provider::elements.find(name);
        if (it != rbus_provider::elements.end() ) {
            getData(it->second,property);
            return (rbusError_t::RBUS_ERROR_SUCCESS);
        }
    }
    return (rbusError_t::RBUS_ERROR_BUS_ERROR);
}

void rbus_provider::registerPath(std::string path,rbus_data value)
{
    rbusError_t ec;

    //add into the elements map & set the name 
    auto out = elements.insert({path,value});
    out.first->second.internal_element.name = const_cast<char*> (out.first->first.c_str());

    ec = rbus_regDataElements(rbusHandle,1,&out.first->second.internal_element);
    if(ec != RBUS_ERROR_SUCCESS)
    {
        std::cerr<<"Couldn't register element: "<<path<<std::endl;
        rbus_close(rbusHandle);
        throw;
    }
}

void rbus_provider::registerTables()
{
    //register all tables that have been added to the providers list
    for(auto& table_it : tables)
    {
        table_it.second.open(rbusHandle);
    }
}

rbusError_t rbus_provider::eventSubHandler(UNUSED_CHECK rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName,UNUSED_CHECK  rbusFilter_t filter,UNUSED_CHECK  int32_t interval,UNUSED_CHECK  bool* autoPublish)
{
    std::cout << (action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe") << " for "<< eventName << std::endl;
    return RBUS_ERROR_SUCCESS;
}