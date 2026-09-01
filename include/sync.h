#pragma once
#include <filesystem>
#include <string_view>
#include <boost/process/v1.hpp>
#include <vector>
#include <string>

constexpr std::string_view dest = "steven@100.98.23.73:/home/steven/projects";

namespace rsync
{
    namespace bp = boost::process::v1;
    namespace fs = std::filesystem;

    inline bool rsync(std::string project_name, fs::path file)
    {
        std::string source = fs::absolute(file).string();
        std::string remote_dest = std::string(dest) + "/" + project_name + "/" + file.string();
        
        int res = bp::system(bp::search_path("rsync"), "-avz", "-e", "ssh", source, remote_dest);
        if(res == 0) return false;
        
        return true;
    }

    inline bool rsync(std::string project_name, std::vector<fs::path> files)
    {
        std::string source = "";
        std::string remote_dest = std::string(dest) + "/" + project_name + "/";
        std::string remote_dest_files = "";

        for(const auto& file : files)
        {
            source += fs::absolute(file).string() + " ";
            remote_dest_files += remote_dest + file.string() + " ";
        }
        int res = bp::system(bp::search_path("rsync"), "-avz", "-e", "ssh", source, remote_dest_files);
        if(res == 0) return false;

        return true;
    }
};
