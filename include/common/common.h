#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

#include "logger.h"

void readEnvFile(const std::string& path, std::unordered_map<std::string, std::string>& env)
{
    std::ifstream file(path);
    std::string line;
    while(std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string key, value;
        std::getline(ss, key, '=');
        std::getline(ss, value);
        env[key] = value;
    }
}

std::unordered_map<std::string, std::string> env;