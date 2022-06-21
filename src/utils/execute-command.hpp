#ifndef __EXECUTE_COMMAND_HPP
#define __EXECUTE_COMMAND_HPP

#include <tuple>
#include <string>

auto execute_command(std::string command) -> std::tuple<int, std::string>;

#endif