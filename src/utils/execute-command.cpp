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

#include "execute-command.hpp"
#include <string>
#include <array>

auto execute_command(std::string command) -> std::tuple<int, std::string>{
    std::string ret_content;

    std::array<char, 256> buffer;
    
    auto pipe = popen(command.c_str(), "r");

    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe)) {
        if (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
            ret_content += buffer.data();
    }

    int ret_code = pclose(pipe);

    return std::make_tuple(ret_code, ret_content);
}
