#include "include/shi.h"

#include <exception>

namespace
{
    void printUsage(std::ostream& out, const char* program)
    {
        out << "Usage:\n"
            << "  " << program << " init\n"
            << "  " << program << " add <file>\n"
            << "  " << program << " cat-stage\n"
            << "  " << program << " sync <project-name>\n"
            << "  " << program << " help\n";
    }

    bool hasArgumentCount(int argc, int expected, const char* program,
                          std::string_view command)
    {
        if(argc == expected) return true;

        std::cerr << "Error: '" << command << "' expects "
                  << expected - 1 << " argument"
                  << (expected - 1 == 1 ? "" : "s") << ".\n\n";
        printUsage(std::cerr, program);
        return false;
    }
}

int main(int argc, char** argv)
{
    const char* program = argc > 0 ? argv[0] : "shi";

    if(argc < 2)
    {
        printUsage(std::cerr, program);
        return 2;
    }

    const std::string_view command = argv[1];

    if(command == "help" || command == "--help" || command == "-h")
    {
        printUsage(std::cout, program);
        return 0;
    }

    try
    {
        if(command == "init")
        {
            if(!hasArgumentCount(argc, 2, program, command)) return 2;
            return shi::init(".") ? 0 : 1;
        }

        if(command == "add")
        {
            if(!hasArgumentCount(argc, 3, program, command)) return 2;

            if(!shi::add(argv[2]))
            {
                std::cerr << "Failed to add file: " << argv[2] << '\n';
                return 1;
            }

            std::cout << "Successfully added file to .shi/objects.\n";
            return 0;
        }

        if(command == "cat-stage")
        {
            if(!hasArgumentCount(argc, 2, program, command)) return 2;
            std::cout << shi::catStage();
            return 0;
        }

        if(command == "sync")
        {
            if(!hasArgumentCount(argc, 3, program, command)) return 2;

            if(!shi::sync(argv[2]))
            {
                std::cerr << "Failed to sync files for project: " << argv[2] << '\n';
                return 1;
            }

            std::cout << "Successfully synced files to remote destination for project: "
                      << argv[2] << '\n';
            return 0;
        }

        std::cerr << "Unknown command: " << command << "\n\n";
        printUsage(std::cerr, program);
        return 2;
    }
    catch(const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
