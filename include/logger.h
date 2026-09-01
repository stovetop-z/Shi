#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace logger
{
    enum class level 
    {
        INFO,
        WARN,
        ERROR
    };

    inline void log(level l, std::string_view msg)
    {
        switch (l)
        {
            case level::INFO:
                std::cout << "[INFO] " << msg << '\n';
                break;
            case level::WARN:
                std::cout << "[WARN] " << msg << '\n';
                break;
            case level::ERROR:
                std::cerr << "[ERROR] " << msg << '\n';
                break;
        }
    }

    inline std::string msgFormat(std::string_view msg, std::string_view function, std::string_view file, int line)
    {
        return "[" + std::string(file) + ":" + std::to_string(line) + " in " + std::string(function) + "()] " + std::string(msg);
    }
}