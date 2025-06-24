//
// Created by Slava Brosgol on 24/06/2025.
//
#pragma once
#include "core/CommandRegistry.hpp"
#include "../utils/YamlToJson.hpp"
#include <yaml-cpp/yaml.h>
#include <cpprest/json.h>
#include <functional>
#include <vector>
#include <string>

struct WorkflowStepResult {
    bool success = true;
    std::string error_message;
    web::json::value output;
};

class WorkflowRunner {
public:
    explicit WorkflowRunner(CommandRegistry& registry) : registry_(registry) {}

    // Returns true if success, fills step_results with per-step results
    bool run(const std::string& yaml_path, std::vector<WorkflowStepResult>& step_results);

    // For HTTP: returns JSON array of step results
    static web::json::value run_and_report(const std::string& yaml_content);

private:
    CommandRegistry& registry_;
    // For result piping ({{stepN.output.field}})
    web::json::value resolve_placeholders(const web::json::value& input, const std::vector<WorkflowStepResult>& previous_results);
};
