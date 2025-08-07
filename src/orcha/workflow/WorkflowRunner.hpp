#pragma once

#include <cpprest/json.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <string>
#include <pplx/pplxtasks.h>
#include "core/CommandRegistry.hpp"

namespace Orcha::Workflow {

    // Simple struct to store per-step results
    struct WorkflowStepResult {
        bool success = false;
        std::string error_message;
        web::json::value output;
    };

    class WorkflowRunner {
    public:
        explicit WorkflowRunner(Orcha::Core::CommandRegistry& registry)
            : registry_(registry) {}

        // Synchronous CLI usage (returns true on success, fills step_results)
        bool run(const std::string& yaml_path, std::vector<WorkflowStepResult>& step_results) const;

        // Asynchronous: returns a task with a JSON array of results (for HTTP/REST)
        [[nodiscard]] pplx::task<web::json::value> run_and_report_async(const std::string& yaml_content) const;
        [[nodiscard]] pplx::task<web::json::value> run_and_report_json(const web::json::value& workflow_json) const;

        // Placeholder resolution (used internally)
        static web::json::value resolve_placeholders(const web::json::value& input, 
                                                      const std::vector<WorkflowStepResult>& previous_results);

    private:
        Orcha::Core::CommandRegistry& registry_;
    };

} // namespace Orcha::Workflow

