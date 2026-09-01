#pragma once
#include <string>
#include <unordered_map>

namespace args
{
    enum class arg_type
    {
        COMMAND,
        UNKNOWN,
        FLAG
    };

    inline arg_type getArgType(const std::string& arg)
    {
        if(arg == "init") return arg_type::COMMAND;
        else if(arg == "add") return arg_type::COMMAND;
        else if(arg == "commit") return arg_type::COMMAND;
        else if(arg == "log") return arg_type::COMMAND;
        else if(arg == "hash") return arg_type::COMMAND;
        else if(arg == "cat") return arg_type::COMMAND;
        else if(arg == "pull") return arg_type::COMMAND;
        else if(arg == "push") return arg_type::COMMAND;
        else if(arg == "compare") return arg_type::COMMAND;
        else if(arg == "sync") return arg_type::COMMAND;
        else if (arg == "-p" || arg == "--path") return arg_type::FLAG;
        else if (arg == "-f" || arg == "--file") return arg_type::FLAG;
        else if (arg == "-m" || arg == "--message") return arg_type::FLAG;
        else return arg_type::UNKNOWN;
    }

    std::unordered_map<std::string, arg_type> parseAndLex(const std::string& args)
    {
        std::unordered_map<std::string, arg_type> parsed;
        std::string prev_arg = "";
        std::string arg = "";

        for(const char& c : args)
        {
            if(c == ' ')
            {
                if(!arg.empty())
                {
                    if(prev_arg == arg || getArgType(arg) == arg_type::UNKNOWN)
                    {
                        logger::log(logger::level::ERROR, logger::msgFormat("Invalid argument: " + arg, __FUNCTION__, __FILE__, __LINE__));
                        return {};
                    }

                    arg_type targ = getArgType(arg);
                    if(parsed.find(arg) == parsed.end())
                    {
                        parsed[arg] = targ;
                    }
                    prev_arg = arg;
                    arg.clear();
                }
            }
            else
            {
                arg += c;
            }
        }

        return parsed;
    }
}