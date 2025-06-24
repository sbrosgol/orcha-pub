//
// Created by Slava Brosgol on 24/06/2025.
//

#pragma once
#include <cpprest/json.h>
#include <string>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual std::string name() const = 0;
    virtual web::json::value execute(const web::json::value& params) = 0;
    virtual void rollback(const web::json::value&) {}
};

extern "C" {
    typedef ICommand* (*CreateCommandFunc)();
}
