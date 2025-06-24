#include "core/CommandRegistry.hpp"
#include "agent/CommandAgent.hpp"
#include "workflow/WorkflowRunner.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    CommandRegistry registry;
    std::string commands_dir = "./commands";
    std::string ext =
#if defined(_WIN32)
        ".dll";
#elif defined(__APPLE__)
            ".dylib";
#else
                ".so";
#endif

    // Recursively load all command plugins under commands/
    for (const auto& entry : fs::recursive_directory_iterator(commands_dir)) {
        if (entry.path().extension() == ext)
            registry.load_command_library(entry.path().string());
    }

    if (argc > 1) {
        std::vector<WorkflowStepResult> step_results;
        WorkflowRunner runner(registry);
        runner.run(argv[1], step_results);
        for (size_t i = 0; i < step_results.size(); ++i) {
            std::cout << "Step " << (i+1) << ": success=" << step_results[i].success
                << " error=" << step_results[i].error_message << " output=" << step_results[i].output.serialize() << std::endl;
        }
        return 0;
    }

    CommandAgent agent(registry);
    agent.start(8070);

    std::cout << "Press Enter to exit...\n";
    std::cin.get();

    agent.stop();
    return 0;
}
