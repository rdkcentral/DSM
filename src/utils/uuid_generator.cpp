
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

#include "uuid_generator.hpp"

#include <set>
#include <string>

#include <cstring>
#include <cstdlib>


std::set<std::string> GENERATED_UUIDS;

std::string generate_UUID(unsigned int uuid_length){
    const char validChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567890-._";
    auto  validCharsNumber = std::strlen(validChars);
    char candidate_raw[uuid_length+1] = {};
    std::string candidate = "";
    do{        
        for (unsigned int i = 0; i < uuid_length; i++){
            candidate_raw[i]=validChars[std::rand()%validCharsNumber];
        }
        candidate = candidate_raw;
    } while ( candidate.length() > 0 && 
              (GENERATED_UUIDS.find(candidate) != GENERATED_UUIDS.end()) );
    GENERATED_UUIDS.insert(candidate);
    return candidate;
}
