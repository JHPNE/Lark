#pragma once
#include <filesystem>

namespace fs = std::filesystem;

class FileSystem {
public:
    static bool SetHidden(const fs::path& path, bool hidden = true) {
        try {
            // Use the <filesystem> library to set file attributes
            fs::perms permissions = fs::status(path).permissions();
            if (hidden) {
                permissions &= ~fs::perms::others_read;
            }
            else {
                permissions |= fs::perms::others_read;
            }
            fs::permissions(path, permissions);
            return true;
        }
        catch (const std::exception& e) {
            return false;
        }
    }

    static bool IsHidden(const fs::path& path) {
        try {
            // Use the <filesystem> library to check file attributes
            fs::perms permissions = fs::status(path).permissions();
            return (permissions & fs::perms::others_read) == fs::perms::none;
        }
        catch (const std::exception& e) {
            return false;
        }
    }
};