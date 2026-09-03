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

void completePattern(const std::string& pattern)
{
    // Implementation for completing patterns with wildcards
    std::string prefix = pattern.substr(0, pattern.find("*"));
    std::string suffix = pattern.substr(pattern.find("*") + 1);

    
}

void readIgnoreFile(const std::string& path, std::vector<std::string>& ignore_patterns)
{
    ignore_patterns.push_back(".shi/");

    std::ifstream file(path);
    std::string line;
    while(std::getline(file, line))
    {
        if(line.find("*") != std::string::npos)
        {
            completePattern(line);
        }
        ignore_patterns.push_back(line);
    }
}

std::unordered_map<std::string, std::string> env;
std::vector<std::string> ignore_patterns;