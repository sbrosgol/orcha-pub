//
// Created by Slava Brosgol on 24/06/2025.
//
#pragma once
#include "ICommand.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

class CommandRegistry {
public:
    ~CommandRegistry();
    bool load_command_library(const std::string& path);
    [[nodiscard]] ICommand* get_command(const std::string& name) const;
private:
    struct PluginHandle {
        void* handle = nullptr;
        std::unique_ptr<ICommand> command;
    };
    std::vector<PluginHandle> plugins_;
    std::unordered_map<std::string, ICommand*> commands_;
};
