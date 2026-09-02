#pragma once
#include <filesystem>
#include <string_view>
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/asio/io_context.hpp>
#include <vector>
#include <string>

constexpr std::string_view remote_root = "steven@100.98.23.73:projects";

namespace rsync
{
    namespace bp = boost::process;
    namespace fs = std::filesystem;

    inline bool rsync(std::string project_name, std::vector<fs::path> files) 
    { 
        if (files.empty()) return true;

        // Destination folder on the remote server
        std::string remote_dest = std::string(remote_root) + "/" + project_name + "/";
        
        std::vector<std::string> args = {"-avzR", "-e", "ssh"};
        for(const auto& file : files) { 
            args.push_back(file.string()); 
            logger::log(logger::level::INFO, logger::msgFormat("Queueing file for sync: " + file.string(), __FUNCTION__, __FILE__, __LINE__));
        }
        
        args.push_back(remote_dest);
        
        logger::log(logger::level::INFO, logger::msgFormat("Executing batch rsync for all files...", __FUNCTION__, __FILE__, __LINE__));

        boost::asio::io_context io_context; 
        bp::v2::process process(io_context, bp::environment::find_executable("rsync"), args); 
        process.wait(); 
        
        int res = process.exit_code(); 
        if(res != 0) {
            logger::log(logger::level::ERROR, logger::msgFormat("Rsync batch failed with exit code: " + std::to_string(res), __FUNCTION__, __FILE__, __LINE__));
            return false; 
        }
        
        return true; 
    }

};
