#include "pch.h"
#include "editor.hpp"

int main()
{
    try 
    {
        Editor editor;
        editor.run();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}