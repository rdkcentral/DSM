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
