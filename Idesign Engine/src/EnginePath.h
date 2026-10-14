#pragma once

#include <string>
#include <filesystem>

// This function guarantees our path is initialized only once,
// the very first time it's called.
inline std::string& GetEngineRootPath()
{
    // This static variable is created and set only once.
    static std::string s_RootPath = std::filesystem::current_path().string();
    return s_RootPath;
}