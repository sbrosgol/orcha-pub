#include "core/CommandRegistry.hpp"
#include "agent/CommandAgent.hpp"
#include "workflow/WorkflowRunner.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    Logger::instance().set_log_file("./orcha.log");
    Logger::instance().log(LogLevel::INFO, "Starting Orcha...");
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

    // Ensure commands directory exists
    if (!fs::exists(commands_dir)) {
        std::cout << "[Orcha] Creating missing commands directory: " << commands_dir << std::endl;
        fs::create_directory(commands_dir);
    }

    // Load all shared library plugins in commands/
    bool found_plugin = false;
    for (const auto& entry : fs::recursive_directory_iterator(commands_dir)) {
        if (entry.path().extension() == ext) {
            registry.load_command_library(entry.path().string());
            found_plugin = true;
        }
    }
    if (!found_plugin) {
        std::cout << "[Orcha] No command plugins found in " << commands_dir
                  << ". Place plugin libraries here to extend Orcha.\n";
    }

    // CLI workflow mode
    if (argc > 1) {
        std::vector<WorkflowStepResult> step_results;
        WorkflowRunner runner(registry);
        if (!runner.run(argv[1], step_results)) {
            std::cout << "[Orcha] Workflow failed!\n";
        }
        for (size_t i = 0; i < step_results.size(); ++i) {
            std::cout << "Step " << (i+1) << ": success=" << step_results[i].success
                << " error=" << step_results[i].error_message
                << " output=" << step_results[i].output.serialize() << std::endl;
        }
        return 0;
    }

    // HTTP server mode
    CommandAgent agent(registry);
    agent.start(8070);

    std::cout << "[Orcha] Listening on http://localhost:8070/\n";
    std::cout << "[Orcha] Press Enter to exit...\n";

    std::cin.get();


    Logger::instance().log(LogLevel::INFO, "Shutting down Orcha...");
    agent.stop();
    std::cout << "[Orcha] Shutdown complete.\n";
    return 0;
}
