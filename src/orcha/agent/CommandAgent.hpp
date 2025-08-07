#pragma once
#include "../core/CommandRegistry.hpp"
#include "../workflow/WorkflowRunner.hpp"
#include <cpprest/http_listener.h>
#include <memory>

#include "Logger.hpp"

namespace Orcha::Agent {

    class CommandAgent {
    public:
        explicit CommandAgent(Orcha::Core::CommandRegistry& registry);
        void start(unsigned short port);
        void stop() const;

    private:
        void handle_request(web::http::http_request request) const;

        Orcha::Core::CommandRegistry& registry_;
        Orcha::Workflow::WorkflowRunner runner_;
        std::unique_ptr<web::http::experimental::listener::http_listener> listener_;
    };

} // namespace Orcha::Agent
