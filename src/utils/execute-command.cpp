#include "execute-command.hpp"

auto execute_command(std::string command) -> std::tuple<int, std::string>{
    const unsigned int bufferSize = 256;    
    std::string ret_content;

    std::array<char, bufferSize> buffer;
    

    auto pipe = popen(command.c_str(), "r");

    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe)) {
        if (fgets(buffer.data(), bufferSize, pipe) != nullptr)
            ret_content += buffer.data();
    }

    int ret_code = pclose(pipe);

    return std::make_tuple(ret_code, ret_content);
}