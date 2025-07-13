#include "../../core/ICommand.hpp"
#include <cpprest/json.h>
#include <string>

class EchoCommand final : public ICommand {
public:
    web::json::value execute(const web::json::value& params) override {
        const auto msg = params.has_field(U("message"))
        ? params.at(U("message")).as_string() : U("");
        web::json::value result;
        result[U("echoed")] = web::json::value::string(msg);
        return result;
    }
    [[nodiscard]] std::string name() const override { return "echo"; }
};

extern "C" ICommand* create_command() {
    return new EchoCommand();
}
