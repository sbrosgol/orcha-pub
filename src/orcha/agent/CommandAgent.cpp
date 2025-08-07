#include "CommandAgent.hpp"
#include "orcha_pch.hpp"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

namespace Orcha::Agent {

    CommandAgent::CommandAgent(Orcha::Core::CommandRegistry& registry)
        : registry_(registry), runner_(registry) {}

    void CommandAgent::start(unsigned short port) {
        const auto uri_string = utility::conversions::to_string_t("http://0.0.0.0:" + std::to_string(port) + "/");
        const uri_builder uri(uri_string);

        listener_ = std::make_unique<http_listener>(uri.to_uri());
        listener_->support([this](const http_request& request) { handle_request(request); });

        Utils::Logger::info("CommandAgent starting on port " + std::to_string(port));
        try {
            (void)listener_->open().wait();
            std::cout << "[Orcha] CommandAgent listening on port " << port << std::endl;
            Utils::Logger::instance().log(Utils::LogLevel::INFO, 
                "CommandAgent listening on port " + std::to_string(port));
        } catch (const std::exception& ex) {
            std::cerr << "[Orcha] Failed to open HTTP listener on port " << port << ": " << ex.what() << std::endl;
            Utils::Logger::instance().log(Utils::LogLevel::ERROR, 
                std::string("Failed to open HTTP listener on port ") + std::to_string(port) + ": " + ex.what());
            throw;
        }
    }

    void CommandAgent::stop() const {
        if (listener_)
            (void)listener_->close().wait();
    }

    void CommandAgent::handle_request(http_request request) const {
        auto path = request.request_uri().path();
        auto method = request.method();
        Utils::Logger::instance().log(Utils::LogLevel::INFO, 
            "Received " + utility::conversions::to_utf8string(method) + " request at path: " + utility::conversions::to_utf8string(path));

        if (request.method() == methods::GET && (path == U("/") || path.empty())) {
            pplx::create_task([request] {
                http_response resp(status_codes::OK);
                resp.headers().add(header_names::content_type, "text/plain; charset=utf-8");
                resp.set_body("Orcha Command Agent is running.\nPOST your YAML workflow to /workflow");
                (void)request.reply(resp);
                Utils::Logger::instance().log(Utils::LogLevel::DEBUG, "Health check endpoint hit.");
            }).then([](const pplx::task<void>& t) { 
                try { 
                    t.get(); 
                } catch (...) {} 
            });
        } else if (request.method() == methods::POST && path == U("/workflow")) {
            if (auto content_type = request.headers().content_type(); content_type == U("application/json")) {
                request.extract_json().then([this, request](const web::json::value& json) {
                    runner_.run_and_report_json(json)
                        .then([request](const web::json::value& result) {
                            (void)request.reply(status_codes::OK, result);
                        });
                });
            } else {
                // Fallback: treat as YAML
                request.extract_string().then([this, request](const std::string& yaml) {
                    runner_.run_and_report_async(yaml)
                        .then([request, yaml](const web::json::value& result) {
                            (void)request.reply(status_codes::OK, result);
                            Utils::Logger::debug("Workflow YAML received: " + 
                                yaml.substr(0, 100) + (yaml.size() > 100 ? "..." : ""));
                        });
                });
            }
        } else {
            pplx::create_task([request] {
                request.reply(status_codes::NotFound, "Endpoint not found.");
            }).then([](const pplx::task<void>& t) { 
                try { 
                    t.get(); 
                } catch (...) {} 
            });
        }
    }

} // namespace Orcha::Agent
