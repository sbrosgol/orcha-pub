//
// Created by Slava Brosgol on 24/06/2025.
//

#pragma once
#include <cpprest/json.h>
#include <string>


namespace Orcha::Core {
    /**
    * @class ICommand
    * @brief An interface representing a command in the Orcha namespace.
    *
    * ICommand serves as a base interface for defining commands, allowing for
    * a standard structure within the command design pattern. Implementing classes
    * can define specific command behaviors.
    *
    * This class is intended to be extended by other classes to provide
    * concrete implementations of the command logic.
    *
    * Part of the Orcha namespace.
    */
    class ICommand {
    public:
        virtual ~ICommand() = default;

        [[nodiscard]] virtual std::string name() const = 0;

        virtual web::json::value execute(const web::json::value &params) = 0;

        virtual void rollback(const web::json::value &) {
        }
    };

    extern "C" {
        typedef ICommand * (*CreateCommandFunc)();
    }
}
