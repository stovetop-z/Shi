#include "include/shi.h"
#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <init|add> [path]\n";
        return 2;
    }

    const std::string command = argv[1];
    if(command == "init")
        return shi::init(".") ? 0 : 1;

    if(command == "add")
    {
        if(argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " add <path>\n";
            return 2;
        }

        if(shi::add(argv[2]))
        {
            std::cout << "Successfully added file to .shi/objects." << std::endl;
            return 0;
        }

        std::cerr << "Failed to add file to .shi/objects." << std::endl;
        return 1;
    }

    if(command == "cat-stage")
    {
        std::cout << shi::catStage() << std::endl;
        return 0;
    }

    if(command == "sync")
    {
        if(argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " sync <project_name>\n";
            return 2;
        }

        if(shi::sync(argv[2]))
        {
            std::cout << "Successfully synced files to remote destination for project: " << argv[2] << std::endl;
            return 0;
        }

        std::cerr << "Failed to sync files to remote destination for project: " << argv[2] << std::endl;
        return 1;
    }

    std::cerr << "Unknown command: " << command << '\n';
    return 2;
}
