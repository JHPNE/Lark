#pragma once
#include "Logger.h"
#include <filesystem>

void Logger::Log(MessageType type, const std::string &msg, const std::string &file,
                 const std::string &caller, int line)
{
    m_messages.emplace_back(type, msg, std::filesystem::path(file).filename().string(), caller,
                            line);
}

void Logger::Clear() { m_messages.clear(); }
