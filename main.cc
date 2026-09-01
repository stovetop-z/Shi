#include "include/shi.h"
#include <iostream>

int main(int args, char** argv)
{
    shi::init(".");

    if(shi::add(argv[2]))
    {
        std::cout << "Successfully added file to .shi/objects." << std::endl;
    }
    else
    {
        std::cerr << "Failed to add file to .shi/objects." << std::endl;
    }

    return 0;
}
