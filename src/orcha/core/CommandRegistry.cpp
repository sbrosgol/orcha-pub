#include "CommandRegistry.hpp"
#include <dlfcn.h>

Orcha::Core::CommandRegistry::~CommandRegistry() {
    for (auto& plugin : plugins_) {
        plugin.command.reset();
        if (plugin.handle) dlclose(plugin.handle);
    }
}

bool Orcha::Core::CommandRegistry::load_command_library(const std::string& path) {
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path << " (" << dlerror() << ")" << std::endl;
        return false;
    }
    dlerror();
    const auto create_func = (CreateCommandFunc)dlsym(handle, "create_command");
    if (const char* dlsym_error = dlerror()) {
        std::cerr << "Cannot load symbol 'create_command' in " << path << ": " << dlsym_error << std::endl;
        dlclose(handle);
        return false;
    }
    std::unique_ptr<ICommand> cmd(create_func());
    if (!cmd) {
        std::cerr << "Plugin in " << path << " did not return a valid command." << std::endl;
        dlclose(handle);
        return false;
    }
    commands_[cmd->name()] = cmd.get();
    plugins_.push_back({handle, std::move(cmd)});
    std::cout << "Loaded command: " << plugins_.back().command->name() << " from " << path << std::endl;
    return true;
}

Orcha::Core::ICommand*Orcha::Core::CommandRegistry::get_command(const std::string& name) const {
    const auto it = commands_.find(name);
    return (it != commands_.end()) ? it->second : nullptr;
}
