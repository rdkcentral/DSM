#include "file-system.hpp"

#include <sys/stat.h>
#include <stdio.h>

auto path_exists(const std::string path) -> bool{
    struct stat buffer;

  return (stat (path.c_str(), &buffer) == 0);
}
