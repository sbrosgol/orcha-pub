#pragma once
#include "../core/CommandRegistry.hpp"
#include "../workflow/WorkflowRunner.hpp"
#include <cpprest/http_listener.h>
#include <memory>

class CommandAgent {
public:
    CommandAgent(CommandRegistry& registry);
    void start(unsigned short port);
    void stop();
private:
    void handle_request(web::http::http_request request);

    CommandRegistry& registry_;
    WorkflowRunner runner_;
    std::unique_ptr<web::http::experimental::listener::http_listener> listener_;
};
