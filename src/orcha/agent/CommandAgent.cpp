#include "CommandAgent.hpp"
#include <cpprest/json.h>
#include <iostream>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

CommandAgent::CommandAgent(CommandRegistry& registry)
    : registry_(registry), runner_(registry) {}

void CommandAgent::start(unsigned short port) {
    uri_builder uri(U("http://0.0.0.0:" + std::to_string(port) + "/"));
    listener_ = std::make_unique<http_listener>(uri.to_uri());
    listener_->support([this](http_request request) { handle_request(request); });
    listener_->open().wait();
    std::cout << "CommandAgent listening on port " << port << std::endl;
}

void CommandAgent::stop() {
    if (listener_) listener_->close().wait();
}

void CommandAgent::handle_request(http_request request) {
    if (request.method() == methods::POST && request.request_uri().path() == U("/workflow")) {
        request.extract_string().then([this, request](const std::string& yaml) mutable {
            auto result = runner_.run_and_report(yaml);
            request.reply(status_codes::OK, result);
        });
    } else {
        request.reply(status_codes::NotFound, "Endpoint not found.");
    }
}
