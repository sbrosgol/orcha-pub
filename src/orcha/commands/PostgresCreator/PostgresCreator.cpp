#include "../../core/ICommand.hpp"
#include <cpprest/json.h>
#include <pqxx/pqxx>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace {
    // Validates a PostgreSQL identifier (db name, owner, template).
    // Allows alphanumeric + underscore, must start with letter or underscore,
    // max 63 chars. The identifier is then safe to splice into quoted
    // CREATE DATABASE SQL with double-quotes.
    bool is_valid_pg_identifier(const std::string& name) {
        if (name.empty() || name.length() > 63) return false;
        static const std::regex valid_identifier(R"(^[a-zA-Z_][a-zA-Z0-9_]*$)");
        return std::regex_match(name, valid_identifier);
    }

    // Escape a value for embedding in a libpq conninfo string. Values
    // containing whitespace, single quote, or backslash must be wrapped in
    // single quotes with the special chars backslash-escaped.
    std::string conninfo_escape(const std::string& v) {
        if (v.empty()) return "''";
        bool needs_quote = false;
        for (char c : v) {
            if (c == ' ' || c == '\'' || c == '\\' || c == '\t' || c == '\n') {
                needs_quote = true;
                break;
            }
        }
        if (!needs_quote) return v;
        std::string out = "'";
        for (char c : v) {
            if (c == '\'' || c == '\\') out += '\\';
            out += c;
        }
        out += "'";
        return out;
    }
}

class PostgresCreator final : public Orcha::Core::ICommand {
public:
    [[nodiscard]] std::string name() const override { return "create_pg_db"; }

    web::json::value execute(const web::json::value& params) override {
        using namespace web;
        using namespace utility;

        json::value result;
        try {
            auto get_str = [&](const char* key, const std::string& def = std::string()) -> std::string {
                if (params.has_field(conversions::to_string_t(key))) {
                    return conversions::to_utf8string(
                        params.at(conversions::to_string_t(key)).as_string());
                }
                return def;
            };
            auto get_bool = [&](const char* key, bool def) -> bool {
                if (params.has_field(conversions::to_string_t(key))) {
                    const auto& v = params.at(conversions::to_string_t(key));
                    if (v.is_boolean()) return v.as_bool();
                    if (v.is_string()) {
                        const auto s = conversions::to_utf8string(v.as_string());
                        return s == "1" || s == "true" || s == "TRUE" || s == "yes";
                    }
                }
                return def;
            };

            const std::string dbname = get_str("dbname");
            if (dbname.empty()) {
                throw std::runtime_error("'dbname' parameter is required");
            }
            if (!is_valid_pg_identifier(dbname)) {
                throw std::runtime_error(
                    "'dbname' contains invalid characters (use alphanumeric and underscores only)");
            }

            const std::string host = get_str("host", "localhost");
            const std::string port = get_str("port", "5432");
            const std::string user = get_str("user", "");
            const std::string password = get_str("password", "");
            const std::string owner = get_str("owner", "");
            const std::string template_db = get_str("template", "");
            const bool if_not_exists = get_bool("if_not_exists", true);

            if (!owner.empty() && !is_valid_pg_identifier(owner)) {
                throw std::runtime_error(
                    "'owner' contains invalid characters (use alphanumeric and underscores only)");
            }
            if (!template_db.empty() && !is_valid_pg_identifier(template_db)) {
                throw std::runtime_error(
                    "'template' contains invalid characters (use alphanumeric and underscores only)");
            }

            // Build the conninfo. We always connect to the "postgres"
            // maintenance database -- you can't run CREATE DATABASE from
            // within a session attached to a different db you're creating.
            std::ostringstream conninfo;
            conninfo << "host=" << conninfo_escape(host)
                     << " port=" << conninfo_escape(port);
            if (!user.empty()) conninfo << " user=" << conninfo_escape(user);
            if (!password.empty()) conninfo << " password=" << conninfo_escape(password);
            conninfo << " dbname=postgres";

            pqxx::connection conn(conninfo.str());

            // Existence check via parameterized query (no SQL injection).
            bool exists = false;
            {
                pqxx::work tx(conn);
                const auto r = tx.exec(
                    "SELECT 1 FROM pg_database WHERE datname = $1",
                    pqxx::params{dbname});
                exists = !r.empty();
                tx.commit();
            }

            if (exists) {
                if (if_not_exists) {
                    result[U("success")] = json::value(true);
                    result[U("dbname")] = json::value::string(conversions::to_string_t(dbname));
                    result[U("created")] = json::value(false);
                    return result;
                }
                throw std::runtime_error("Database already exists: " + dbname);
            }

            // CREATE DATABASE cannot run inside a transaction block; use a
            // nontransaction. Identifiers are already validated and spliced
            // with double-quotes so this is safe.
            std::ostringstream sql;
            sql << "CREATE DATABASE \"" << dbname << "\"";
            if (!owner.empty())       sql << " OWNER \"" << owner << "\"";
            if (!template_db.empty()) sql << " TEMPLATE \"" << template_db << "\"";

            {
                pqxx::nontransaction ntx(conn);
                ntx.exec(sql.str());
            }

            result[U("success")] = json::value(true);
            result[U("dbname")] = json::value::string(conversions::to_string_t(dbname));
            result[U("created")] = json::value(true);
            return result;
        } catch (const pqxx::sql_error& ex) {
            std::cerr << "PostgreSQL error: " << ex.what() << " [query: " << ex.query() << "]\n";
            result[U("success")] = json::value(false);
            result[U("error")] = json::value::string(
                conversions::to_string_t(std::string("PostgreSQL error: ") + ex.what()));
            return result;
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << '\n';
            result[U("success")] = json::value(false);
            result[U("error")] = json::value::string(conversions::to_string_t(ex.what()));
            return result;
        }
    }
};

extern "C" Orcha::Core::ICommand* create_command() {
    return new PostgresCreator();
}
