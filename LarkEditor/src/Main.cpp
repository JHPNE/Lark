#pragma once
#include "Editor/EditorApplication.h"
#include <iostream>

int main()
{
    ::editor::EditorApplication &app = ::editor::EditorApplication::Get();

    if (!app.Initialize())
    {
        std::cerr << "Failed to initialize editor application" << std::endl;
        return -1;
    }

    app.Run();
    app.Shutdown();

    return 0;
}