#include "CommandAgent.hpp"
#include "orcha_pch.hpp"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

CommandAgent::CommandAgent(CommandRegistry& registry)
    : registry_(registry), runner_(registry) {}

void CommandAgent::start(unsigned short port) {
    auto uri_string = utility::conversions::to_string_t("http://0.0.0.0:" + std::to_string(port) + "/");
    const uri_builder uri(uri_string);

    listener_ = std::make_unique<http_listener>(uri.to_uri());
    listener_->support([this](const http_request& request) { handle_request(request); });

    try {
        (void)listener_->open().wait();
        std::cout << "[Orcha] CommandAgent listening on port " << port << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "[Orcha] Failed to open HTTP listener on port " << port << ": " << ex.what() << std::endl;
        throw;
    }
}

void CommandAgent::stop() const {
    if (listener_)
        (void)listener_->close().wait();
}

void CommandAgent::handle_request(http_request request) {
    auto path = request.request_uri().path();

    if (request.method() == methods::GET && (path == U("/") || path.empty())) {
        pplx::create_task([request] {
            http_response resp(status_codes::OK);
            resp.headers().add(header_names::content_type, "text/plain; charset=utf-8");
            resp.set_body("Orcha Command Agent is running.\nPOST your YAML workflow to /workflow");
            (void)request.reply(resp);
        }).then([](pplx::task<void> t) { try { t.get(); } catch (...) {} });
    }
    else if (request.method() == methods::POST && path == U("/workflow")) {
        request.extract_string().then([this, request](const std::string& yaml) {
            runner_.run_and_report_async(yaml)
                .then([request](web::json::value result) {
                    (void)request.reply(status_codes::OK, result);
                })
                .then([](pplx::task<void> t) {
                    try { t.get(); } catch (const std::exception& ex) {
                        std::cerr << "[Orcha] Exception in workflow execution: " << ex.what() << std::endl;
                    }
                });
        });
    } else {
        pplx::create_task([request] {
            request.reply(status_codes::NotFound, "Endpoint not found.");
        }).then([](pplx::task<void> t) { try { t.get(); } catch (...) {} });
    }
}
