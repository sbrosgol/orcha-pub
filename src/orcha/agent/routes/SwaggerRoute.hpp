//
// SwaggerRoute.hpp - Swagger/OpenAPI endpoint handlers
//

#pragma once

#include "../IRouteHandler.hpp"
#include "../swagger_embedded.hpp"
#include "../../core/ICommandRegistry.hpp"
#include "../../utils/ILogger.hpp"
#include "core/Version.hpp"

namespace Orcha::Agent::Routes {

    /**
     * @class SwaggerRoute
     * @brief Handles Swagger UI and OpenAPI spec endpoints.
     */
    class SwaggerRoute : public IRouteHandler {
    public:
        SwaggerRoute(std::shared_ptr<Core::ICommandRegistry> registry,
                    std::shared_ptr<Utils::ILogger> logger = nullptr)
            : registry_(std::move(registry))
            , logger_(std::move(logger)) {}

        [[nodiscard]] bool can_handle(
            const std::string& method,
            const std::string& path) const override {
            if (method != "GET") return false;
            return path == "/swagger" ||
                   path == "/swagger.json" ||
                   path == "/sample" ||
                   path == "/commands";
        }

        void handle(web::http::http_request request) override {
            auto path = utility::conversions::to_utf8string(request.request_uri().path());

            if (path == "/swagger") {
                serve_swagger_ui(request);
            } else if (path == "/swagger.json") {
                serve_openapi_spec(request);
            } else if (path == "/sample") {
                serve_sample_workflow(request);
            } else if (path == "/commands") {
                serve_commands_list(request);
            }
        }

        [[nodiscard]] std::vector<RouteInfo> get_routes() const override {
            return {
                {.method = "GET", .path = "/swagger", .description = "Swagger UI"},
                {.method = "GET", .path = "/swagger.json", .description = "OpenAPI specification"},
                {.method = "GET", .path = "/sample", .description = "Sample workflow payload"},
                {.method = "GET", .path = "/commands", .description = "List available commands"}
            };
        }

    private:
        void serve_swagger_ui(web::http::http_request request) {
            pplx::create_task([request]() {
                web::http::http_response resp(web::http::status_codes::OK);
                resp.headers().add(
                    web::http::header_names::content_type,
                    U("text/html; charset=utf-8"));
                resp.set_body(Orcha::Agent::kSwaggerHtml);
                request.reply(resp);
            });
        }

        void serve_openapi_spec(web::http::http_request request) {
            auto registry = registry_;

            pplx::create_task([request, registry]() {
                auto spec = generate_openapi_spec(registry);
                web::http::http_response resp(web::http::status_codes::OK);
                resp.headers().add(
                    web::http::header_names::content_type,
                    U("application/json"));
                resp.set_body(spec);
                request.reply(resp);
            });
        }

        void serve_sample_workflow(web::http::http_request request) {
            pplx::create_task([request]() {
                web::json::value sample = web::json::value::object();
                web::json::value steps = web::json::value::array(1);

                web::json::value step0 = web::json::value::object();
                step0[U("command")] = web::json::value::string(U("echo"));
                web::json::value params = web::json::value::object();
                params[U("message")] = web::json::value::string(U("Hello, Orcha!"));
                step0[U("params")] = params;
                steps[0] = step0;

                sample[U("steps")] = steps;

                web::http::http_response resp(web::http::status_codes::OK);
                resp.headers().add(
                    web::http::header_names::content_type,
                    U("application/json"));
                resp.set_body(sample);
                request.reply(resp);
            });
        }

        void serve_commands_list(web::http::http_request request) {
            auto registry = registry_;

            pplx::create_task([request, registry]() {
                web::json::value result = web::json::value::object();

                auto commands = registry->list_commands();
                web::json::value arr = web::json::value::array(commands.size());

                for (size_t i = 0; i < commands.size(); ++i) {
                    const auto& name = commands[i];
                    web::json::value cmd_info = web::json::value::object();
                    cmd_info[U("name")] = web::json::value::string(name);

                    if (auto cmd = registry->get_command(name)) {
                        auto meta = cmd->metadata();
                        cmd_info[U("version")] = web::json::value::string(meta.version);
                        cmd_info[U("description")] = web::json::value::string(meta.description);

                        if (!meta.parameters.empty()) {
                            web::json::value params = web::json::value::array(meta.parameters.size());
                            for (size_t j = 0; j < meta.parameters.size(); ++j) {
                                params[j] = meta.parameters[j].to_json();
                            }
                            cmd_info[U("parameters")] = params;
                        }
                    }

                    arr[i] = cmd_info;
                }

                result[U("commands")] = arr;
                result[U("count")] = web::json::value::number(static_cast<int>(commands.size()));

                web::http::http_response resp(web::http::status_codes::OK);
                resp.headers().add(
                    web::http::header_names::content_type,
                    U("application/json"));
                resp.set_body(result);
                request.reply(resp);
            });
        }

        // ====================================================================
        // OpenAPI spec generation
        // ====================================================================

        static web::json::value generate_openapi_spec(
            std::shared_ptr<Core::ICommandRegistry> registry) {

            using web::json::value;

            value spec = value::object();
            spec[U("openapi")] = value::string(U("3.0.3"));
            spec[U("info")] = build_info();
            spec[U("servers")] = build_servers();
            spec[U("tags")] = build_tags();
            spec[U("components")] = build_components();
            spec[U("paths")] = build_paths(registry);
            return spec;
        }

        // ---- Info / Servers / Tags ----------------------------------------

        static web::json::value build_info() {
            using web::json::value;
            value info = value::object();
            info[U("title")] = value::string(U("Orcha API"));
            info[U("version")] = value::string(Orcha::kVersion);
            info[U("description")] = value::string(
                U("Orcha is a plugin-based command orchestration engine. ")
                U("Public endpoints expose workflow execution and discovery; ")
                U("`/api/*` endpoints require Basic auth and are used by the ")
                U("admin dashboard at `/admin`."));

            value license = value::object();
            license[U("name")] = value::string(U("MIT"));
            info[U("license")] = license;
            return info;
        }

        static web::json::value build_servers() {
            using web::json::value;
            value servers = value::array(1);
            value server0 = value::object();
            server0[U("url")] = value::string(U("/"));
            server0[U("description")] = value::string(U("Current host"));
            servers[0] = server0;
            return servers;
        }

        static web::json::value build_tags() {
            using web::json::value;
            const std::pair<const char*, const char*> entries[] = {
                {"System",   "Health and discovery"},
                {"Workflow", "Ad-hoc workflow execution"},
                {"Commands", "Available command plugins"},
                {"Jobs",     "Saved workflow definitions (admin)"},
                {"Runs",     "Execution history (admin)"},
                {"Plugins",  "Plugin lifecycle (admin)"},
            };
            value tags = value::array(std::size(entries));
            for (size_t i = 0; i < std::size(entries); ++i) {
                value t = value::object();
                t[U("name")] = value::string(utility::conversions::to_string_t(entries[i].first));
                t[U("description")] = value::string(
                    utility::conversions::to_string_t(entries[i].second));
                tags[i] = t;
            }
            return tags;
        }

        // ---- Components: schemas + security -------------------------------

        static web::json::value build_components() {
            using web::json::value;
            value components = value::object();

            // Security schemes
            value security_schemes = value::object();
            value basic = value::object();
            basic[U("type")] = value::string(U("http"));
            basic[U("scheme")] = value::string(U("basic"));
            basic[U("description")] = value::string(
                U("Admin HTTP Basic credentials (see `admin.username` / `admin.password` in `orcha.yaml`)."));
            security_schemes[U("basicAuth")] = basic;
            components[U("securitySchemes")] = security_schemes;

            // Schemas
            value schemas = value::object();
            schemas[U("Error")]            = schema_error();
            schemas[U("WorkflowStep")]     = schema_workflow_step();
            schemas[U("WorkflowRequest")]  = schema_workflow_request();
            schemas[U("StepResult")]       = schema_step_result();
            schemas[U("WorkflowResult")]   = schema_workflow_result();
            schemas[U("CommandParameter")] = schema_command_parameter();
            schemas[U("CommandInfo")]      = schema_command_info();
            schemas[U("CommandsList")]     = schema_commands_list();
            schemas[U("JobDefinition")]    = schema_job_definition();
            schemas[U("JobInput")]         = schema_job_input();
            schemas[U("JobsList")]         = schema_jobs_list();
            schemas[U("RunRecord")]        = schema_run_record();
            schemas[U("RunsList")]         = schema_runs_list();
            schemas[U("PluginInfo")]       = schema_plugin_info();
            schemas[U("PluginsList")]      = schema_plugins_list();
            schemas[U("PluginActionResult")] = schema_plugin_action_result();
            schemas[U("WatchStatus")]      = schema_watch_status();
            schemas[U("WatchUpdate")]      = schema_watch_update();
            components[U("schemas")] = schemas;

            return components;
        }

        // Convenience: a $ref string.
        static web::json::value ref(const utility::string_t& name) {
            web::json::value r = web::json::value::object();
            r[U("$ref")] = web::json::value::string(U("#/components/schemas/") + name);
            return r;
        }

        // Convenience: { "type": "string" } / "integer" / "boolean" / "object".
        static web::json::value type_of(const utility::string_t& t) {
            web::json::value v = web::json::value::object();
            v[U("type")] = web::json::value::string(t);
            return v;
        }

        static web::json::value type_with_desc(const utility::string_t& t,
                                                const utility::string_t& desc) {
            auto v = type_of(t);
            v[U("description")] = web::json::value::string(desc);
            return v;
        }

        // ---- Schemas -------------------------------------------------------

        static web::json::value schema_error() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("error")] = type_of(U("string"));
            s[U("properties")] = props;
            value req = value::array(1);
            req[0] = value::string(U("error"));
            s[U("required")] = req;
            return s;
        }

        static web::json::value schema_workflow_step() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("command")] = type_with_desc(U("string"),
                U("Registered command name (see GET /commands)"));
            value params = type_of(U("object"));
            params[U("additionalProperties")] = value::boolean(true);
            params[U("description")] = value::string(U("Command-specific parameters"));
            props[U("params")] = params;
            s[U("properties")] = props;
            value req = value::array(1);
            req[0] = value::string(U("command"));
            s[U("required")] = req;
            return s;
        }

        static web::json::value schema_workflow_request() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            value steps = value::object();
            steps[U("type")] = value::string(U("array"));
            steps[U("items")] = ref(U("WorkflowStep"));
            props[U("steps")] = steps;
            s[U("properties")] = props;
            value req = value::array(1);
            req[0] = value::string(U("steps"));
            s[U("required")] = req;
            return s;
        }

        static web::json::value schema_step_result() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("success")] = type_of(U("boolean"));
            props[U("output")] = type_of(U("object"));
            props[U("error")] = type_of(U("string"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_workflow_result() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("array"));
            s[U("items")] = ref(U("StepResult"));
            return s;
        }

        static web::json::value schema_command_parameter() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("name")] = type_of(U("string"));
            props[U("type")] = type_of(U("string"));
            props[U("description")] = type_of(U("string"));
            props[U("required")] = type_of(U("boolean"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_command_info() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("name")] = type_of(U("string"));
            props[U("version")] = type_of(U("string"));
            props[U("description")] = type_of(U("string"));
            value params = value::object();
            params[U("type")] = value::string(U("array"));
            params[U("items")] = ref(U("CommandParameter"));
            props[U("parameters")] = params;
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_commands_list() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            value items = value::object();
            items[U("type")] = value::string(U("array"));
            items[U("items")] = ref(U("CommandInfo"));
            props[U("commands")] = items;
            props[U("count")] = type_of(U("integer"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_job_definition() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("id")] = type_of(U("string"));
            props[U("name")] = type_of(U("string"));
            props[U("description")] = type_of(U("string"));
            props[U("definition")] = ref(U("WorkflowRequest"));
            props[U("enabled")] = type_of(U("boolean"));
            value cron = type_of(U("string"));
            cron[U("nullable")] = value::boolean(true);
            cron[U("description")] = value::string(
                U("Cron expression (5-field, UTC) or null"));
            props[U("schedule_cron")] = cron;
            props[U("created_at")] = type_of(U("string"));
            props[U("updated_at")] = type_of(U("string"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_job_input() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("name")] = type_of(U("string"));
            props[U("description")] = type_of(U("string"));
            props[U("definition")] = ref(U("WorkflowRequest"));
            props[U("enabled")] = type_of(U("boolean"));
            value cron = type_of(U("string"));
            cron[U("nullable")] = value::boolean(true);
            props[U("schedule_cron")] = cron;
            s[U("properties")] = props;
            value req = value::array(2);
            req[0] = value::string(U("name"));
            req[1] = value::string(U("definition"));
            s[U("required")] = req;
            return s;
        }

        static web::json::value schema_jobs_list() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            value items = value::object();
            items[U("type")] = value::string(U("array"));
            items[U("items")] = ref(U("JobDefinition"));
            props[U("jobs")] = items;
            props[U("count")] = type_of(U("integer"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_run_record() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("id")] = type_of(U("string"));
            value jid = type_of(U("string"));
            jid[U("nullable")] = value::boolean(true);
            jid[U("description")] = value::string(U("Null for ad-hoc /workflow runs"));
            props[U("job_id")] = jid;
            value trig = type_of(U("string"));
            value trig_enum = value::array(3);
            trig_enum[0] = value::string(U("manual"));
            trig_enum[1] = value::string(U("api"));
            trig_enum[2] = value::string(U("schedule"));
            trig[U("enum")] = trig_enum;
            props[U("trigger")] = trig;
            value st = type_of(U("string"));
            value st_enum = value::array(2);
            st_enum[0] = value::string(U("success"));
            st_enum[1] = value::string(U("failed"));
            st[U("enum")] = st_enum;
            props[U("status")] = st;
            props[U("started_at")] = type_of(U("string"));
            value fin = type_of(U("string"));
            fin[U("nullable")] = value::boolean(true);
            props[U("finished_at")] = fin;
            props[U("result")] = ref(U("WorkflowResult"));
            props[U("error")] = type_of(U("string"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_runs_list() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            value items = value::object();
            items[U("type")] = value::string(U("array"));
            items[U("items")] = ref(U("RunRecord"));
            props[U("runs")] = items;
            props[U("count")] = type_of(U("integer"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_plugin_info() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("name")] = type_of(U("string"));
            props[U("version")] = type_of(U("string"));
            props[U("description")] = type_of(U("string"));
            props[U("author")] = type_of(U("string"));
            value status = type_of(U("string"));
            value status_enum = value::array(3);
            status_enum[0] = value::string(U("loaded"));
            status_enum[1] = value::string(U("available"));
            status_enum[2] = value::string(U("disabled"));
            status[U("enum")] = status_enum;
            props[U("status")] = status;
            props[U("library_path")] = type_of(U("string"));
            value deps = value::object();
            deps[U("type")] = value::string(U("array"));
            deps[U("items")] = type_of(U("string"));
            props[U("dependencies")] = deps;
            value params = value::object();
            params[U("type")] = value::string(U("array"));
            params[U("items")] = ref(U("CommandParameter"));
            props[U("parameters")] = params;
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_plugins_list() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("directory")] = type_of(U("string"));
            props[U("watching")] = type_of(U("boolean"));
            value items = value::object();
            items[U("type")] = value::string(U("array"));
            items[U("items")] = ref(U("PluginInfo"));
            props[U("plugins")] = items;
            props[U("count")] = type_of(U("integer"));
            value cmds = value::object();
            cmds[U("type")] = value::string(U("array"));
            cmds[U("items")] = type_of(U("string"));
            props[U("commands")] = cmds;
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_plugin_action_result() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("success")] = type_of(U("boolean"));
            props[U("message")] = type_of(U("string"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_watch_status() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("watching")] = type_of(U("boolean"));
            s[U("properties")] = props;
            return s;
        }

        static web::json::value schema_watch_update() {
            using web::json::value;
            value s = value::object();
            s[U("type")] = value::string(U("object"));
            value props = value::object();
            props[U("enabled")] = type_of(U("boolean"));
            s[U("properties")] = props;
            value req = value::array(1);
            req[0] = value::string(U("enabled"));
            s[U("required")] = req;
            return s;
        }

        // ---- Path builders -------------------------------------------------

        // Helper: a JSON response { description, content: { application/json: { schema: $ref } } }
        static web::json::value json_response(const utility::string_t& description,
                                              const utility::string_t& schema_name) {
            using web::json::value;
            value r = value::object();
            r[U("description")] = value::string(description);
            value content = value::object();
            value app_json = value::object();
            app_json[U("schema")] = ref(schema_name);
            content[U("application/json")] = app_json;
            r[U("content")] = content;
            return r;
        }

        static web::json::value text_response(const utility::string_t& description) {
            using web::json::value;
            value r = value::object();
            r[U("description")] = value::string(description);
            value content = value::object();
            value text_plain = value::object();
            value schema = value::object();
            schema[U("type")] = value::string(U("string"));
            text_plain[U("schema")] = schema;
            content[U("text/plain")] = text_plain;
            r[U("content")] = content;
            return r;
        }

        static web::json::value simple_response(const utility::string_t& description) {
            using web::json::value;
            value r = value::object();
            r[U("description")] = value::string(description);
            return r;
        }

        static web::json::value error_response(const utility::string_t& description) {
            return json_response(description, U("Error"));
        }

        // Adds a `security: [{ basicAuth: [] }]` requirement.
        static web::json::value basic_auth_security() {
            using web::json::value;
            value sec = value::array(1);
            value s = value::object();
            s[U("basicAuth")] = value::array(0);
            sec[0] = s;
            return sec;
        }

        static web::json::value tag(const utility::string_t& name) {
            using web::json::value;
            value arr = value::array(1);
            arr[0] = value::string(name);
            return arr;
        }

        static web::json::value path_param(const utility::string_t& name,
                                           const utility::string_t& description) {
            using web::json::value;
            value p = value::object();
            p[U("name")] = value::string(name);
            p[U("in")] = value::string(U("path"));
            p[U("required")] = value::boolean(true);
            p[U("description")] = value::string(description);
            p[U("schema")] = type_of(U("string"));
            return p;
        }

        static web::json::value query_int(const utility::string_t& name,
                                          const utility::string_t& description,
                                          int default_value,
                                          int min_value,
                                          int max_value) {
            using web::json::value;
            value p = value::object();
            p[U("name")] = value::string(name);
            p[U("in")] = value::string(U("query"));
            p[U("required")] = value::boolean(false);
            p[U("description")] = value::string(description);
            value schema = value::object();
            schema[U("type")] = value::string(U("integer"));
            schema[U("default")] = value::number(default_value);
            schema[U("minimum")] = value::number(min_value);
            schema[U("maximum")] = value::number(max_value);
            p[U("schema")] = schema;
            return p;
        }

        static web::json::value json_body(const utility::string_t& schema_name,
                                          bool required = true) {
            using web::json::value;
            value body = value::object();
            body[U("required")] = value::boolean(required);
            value content = value::object();
            value app_json = value::object();
            app_json[U("schema")] = ref(schema_name);
            content[U("application/json")] = app_json;
            body[U("content")] = content;
            return body;
        }

        // ---- All paths -----------------------------------------------------

        static web::json::value build_paths(
            std::shared_ptr<Core::ICommandRegistry> /*registry*/) {
            using web::json::value;
            value paths = value::object();

            // Public
            paths[U("/")]            = path_health();
            paths[U("/workflow")]    = path_workflow();
            paths[U("/commands")]    = path_commands();
            paths[U("/sample")]      = path_sample();

            // Admin: jobs
            paths[U("/api/jobs")]                  = path_jobs_collection();
            paths[U("/api/jobs/{id}")]             = path_job_item();
            paths[U("/api/jobs/{id}/run")]         = path_job_run();
            paths[U("/api/jobs/{id}/runs")]        = path_job_runs();

            // Admin: runs
            paths[U("/api/runs")]                  = path_runs_collection();
            paths[U("/api/runs/{id}")]             = path_run_item();

            // Admin: plugins
            paths[U("/api/plugins")]               = path_plugins_collection();
            paths[U("/api/plugins/{name}")]        = path_plugin_item();
            paths[U("/api/plugins/{name}/reload")] = path_plugin_action(U("reload"), U("Reload"));
            paths[U("/api/plugins/{name}/enable")] = path_plugin_action(U("enable"), U("Enable"));
            paths[U("/api/plugins/{name}/disable")]= path_plugin_action(U("disable"), U("Disable"));
            paths[U("/api/plugins/_watch")]        = path_plugins_watch();

            return paths;
        }

        // -- Public paths ----------------------------------------------------

        static web::json::value path_health() {
            using web::json::value;
            value path = value::object();
            value op = value::object();
            op[U("summary")] = value::string(U("Health check"));
            op[U("description")] = value::string(
                U("Returns a plain-text banner with quick links."));
            op[U("tags")] = tag(U("System"));
            value responses = value::object();
            responses[U("200")] = text_response(U("Banner"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        static web::json::value path_workflow() {
            using web::json::value;
            value path = value::object();
            value op = value::object();
            op[U("summary")] = value::string(U("Execute a workflow"));
            op[U("description")] = value::string(
                U("Executes the supplied steps and returns per-step results."));
            op[U("tags")] = tag(U("Workflow"));
            op[U("requestBody")] = json_body(U("WorkflowRequest"));
            value responses = value::object();
            responses[U("200")] = json_response(U("Workflow result"), U("WorkflowResult"));
            responses[U("415")] = error_response(U("Unsupported media type"));
            responses[U("500")] = error_response(U("Execution error"));
            op[U("responses")] = responses;
            path[U("post")] = op;
            return path;
        }

        static web::json::value path_commands() {
            using web::json::value;
            value path = value::object();
            value op = value::object();
            op[U("summary")] = value::string(U("List available commands"));
            op[U("tags")] = tag(U("Commands"));
            value responses = value::object();
            responses[U("200")] = json_response(U("Registered commands"), U("CommandsList"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        static web::json::value path_sample() {
            using web::json::value;
            value path = value::object();
            value op = value::object();
            op[U("summary")] = value::string(U("Get sample workflow"));
            op[U("description")] = value::string(
                U("Returns a minimal workflow payload usable with POST /workflow."));
            op[U("tags")] = tag(U("Workflow"));
            value responses = value::object();
            responses[U("200")] = json_response(U("Sample workflow"), U("WorkflowRequest"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        // -- Jobs paths ------------------------------------------------------

        static web::json::value path_jobs_collection() {
            using web::json::value;
            value path = value::object();

            // GET
            value get_op = value::object();
            get_op[U("summary")] = value::string(U("List jobs"));
            get_op[U("tags")] = tag(U("Jobs"));
            get_op[U("security")] = basic_auth_security();
            value get_resp = value::object();
            get_resp[U("200")] = json_response(U("Jobs"), U("JobsList"));
            get_resp[U("401")] = error_response(U("Unauthorized"));
            get_op[U("responses")] = get_resp;
            path[U("get")] = get_op;

            // POST
            value post_op = value::object();
            post_op[U("summary")] = value::string(U("Create a job"));
            post_op[U("tags")] = tag(U("Jobs"));
            post_op[U("security")] = basic_auth_security();
            post_op[U("requestBody")] = json_body(U("JobInput"));
            value post_resp = value::object();
            post_resp[U("201")] = json_response(U("Created"), U("JobDefinition"));
            post_resp[U("400")] = error_response(U("Validation error"));
            post_resp[U("401")] = error_response(U("Unauthorized"));
            post_resp[U("409")] = error_response(U("Job name already exists"));
            post_op[U("responses")] = post_resp;
            path[U("post")] = post_op;

            return path;
        }

        static web::json::value path_job_item() {
            using web::json::value;
            value path = value::object();
            value params = value::array(1);
            params[0] = path_param(U("id"), U("Job ID"));
            path[U("parameters")] = params;

            value get_op = value::object();
            get_op[U("summary")] = value::string(U("Get a job"));
            get_op[U("tags")] = tag(U("Jobs"));
            get_op[U("security")] = basic_auth_security();
            value get_resp = value::object();
            get_resp[U("200")] = json_response(U("Job"), U("JobDefinition"));
            get_resp[U("404")] = error_response(U("Not found"));
            get_op[U("responses")] = get_resp;
            path[U("get")] = get_op;

            value put_op = value::object();
            put_op[U("summary")] = value::string(U("Update a job"));
            put_op[U("tags")] = tag(U("Jobs"));
            put_op[U("security")] = basic_auth_security();
            put_op[U("requestBody")] = json_body(U("JobInput"));
            value put_resp = value::object();
            put_resp[U("200")] = json_response(U("Updated"), U("JobDefinition"));
            put_resp[U("400")] = error_response(U("Validation error"));
            put_resp[U("404")] = error_response(U("Not found"));
            put_resp[U("409")] = error_response(U("Name conflict"));
            put_op[U("responses")] = put_resp;
            path[U("put")] = put_op;

            value del_op = value::object();
            del_op[U("summary")] = value::string(U("Delete a job"));
            del_op[U("tags")] = tag(U("Jobs"));
            del_op[U("security")] = basic_auth_security();
            value del_resp = value::object();
            del_resp[U("200")] = simple_response(U("Deleted"));
            del_resp[U("404")] = error_response(U("Not found"));
            del_op[U("responses")] = del_resp;
            path[U("delete")] = del_op;

            return path;
        }

        static web::json::value path_job_run() {
            using web::json::value;
            value path = value::object();
            value params = value::array(1);
            params[0] = path_param(U("id"), U("Job ID"));
            path[U("parameters")] = params;

            value op = value::object();
            op[U("summary")] = value::string(U("Run a job now"));
            op[U("description")] = value::string(
                U("Synchronously executes the job and returns the resulting run record."));
            op[U("tags")] = tag(U("Jobs"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Run record"), U("RunRecord"));
            responses[U("404")] = error_response(U("Not found"));
            op[U("responses")] = responses;
            path[U("post")] = op;
            return path;
        }

        static web::json::value path_job_runs() {
            using web::json::value;
            value path = value::object();
            value params = value::array(2);
            params[0] = path_param(U("id"), U("Job ID"));
            params[1] = query_int(U("limit"), U("Max rows to return"), 50, 1, 1000);
            path[U("parameters")] = params;

            value op = value::object();
            op[U("summary")] = value::string(U("Run history for a job"));
            op[U("tags")] = tag(U("Runs"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Runs"), U("RunsList"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        // -- Runs paths ------------------------------------------------------

        static web::json::value path_runs_collection() {
            using web::json::value;
            value path = value::object();
            value params = value::array(1);
            params[0] = query_int(U("limit"), U("Max rows to return"), 50, 1, 1000);
            path[U("parameters")] = params;

            value op = value::object();
            op[U("summary")] = value::string(U("Recent runs (all jobs + ad-hoc)"));
            op[U("tags")] = tag(U("Runs"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Runs"), U("RunsList"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        static web::json::value path_run_item() {
            using web::json::value;
            value path = value::object();
            value params = value::array(1);
            params[0] = path_param(U("id"), U("Run ID"));
            path[U("parameters")] = params;

            value op = value::object();
            op[U("summary")] = value::string(U("Get a run"));
            op[U("tags")] = tag(U("Runs"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Run"), U("RunRecord"));
            responses[U("404")] = error_response(U("Not found"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        // -- Plugin paths ----------------------------------------------------

        static web::json::value path_plugins_collection() {
            using web::json::value;
            value path = value::object();
            value op = value::object();
            op[U("summary")] = value::string(U("List loaded and available plugins"));
            op[U("tags")] = tag(U("Plugins"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Plugins"), U("PluginsList"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        static web::json::value path_plugin_item() {
            using web::json::value;
            value path = value::object();
            value params = value::array(1);
            params[0] = path_param(U("name"), U("Plugin name"));
            path[U("parameters")] = params;

            value op = value::object();
            op[U("summary")] = value::string(U("Get plugin metadata"));
            op[U("tags")] = tag(U("Plugins"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Plugin"), U("PluginInfo"));
            responses[U("404")] = error_response(U("Plugin not loaded"));
            op[U("responses")] = responses;
            path[U("get")] = op;
            return path;
        }

        static web::json::value path_plugin_action(const utility::string_t& action,
                                                   const utility::string_t& verb) {
            using web::json::value;
            value path = value::object();
            value params = value::array(1);
            params[0] = path_param(U("name"), U("Plugin name"));
            path[U("parameters")] = params;

            value op = value::object();
            op[U("summary")] = value::string(verb + U(" a plugin"));
            op[U("description")] = value::string(
                action == U("enable")
                    ? U("Loads an available plugin from disk and clears any persisted disable.")
                : action == U("disable")
                    ? U("Unloads a plugin and persists it as disabled so it stays off across restarts.")
                    : U("Reloads a loaded plugin (does not touch the denylist)."));
            op[U("tags")] = tag(U("Plugins"));
            op[U("security")] = basic_auth_security();
            value responses = value::object();
            responses[U("200")] = json_response(U("Action succeeded"), U("PluginActionResult"));
            responses[U("404")] = error_response(U("Plugin not found"));
            responses[U("409")] = json_response(U("Action failed"), U("PluginActionResult"));
            op[U("responses")] = responses;
            path[U("post")] = op;
            return path;
        }

        static web::json::value path_plugins_watch() {
            using web::json::value;
            value path = value::object();

            value get_op = value::object();
            get_op[U("summary")] = value::string(U("Directory watcher status"));
            get_op[U("tags")] = tag(U("Plugins"));
            get_op[U("security")] = basic_auth_security();
            value get_resp = value::object();
            get_resp[U("200")] = json_response(U("Watch status"), U("WatchStatus"));
            get_op[U("responses")] = get_resp;
            path[U("get")] = get_op;

            value put_op = value::object();
            put_op[U("summary")] = value::string(U("Enable or disable the watcher"));
            put_op[U("tags")] = tag(U("Plugins"));
            put_op[U("security")] = basic_auth_security();
            put_op[U("requestBody")] = json_body(U("WatchUpdate"));
            value put_resp = value::object();
            put_resp[U("200")] = json_response(U("Watch status"), U("WatchStatus"));
            put_resp[U("400")] = error_response(U("Invalid body"));
            put_op[U("responses")] = put_resp;
            path[U("put")] = put_op;

            return path;
        }

        std::shared_ptr<Core::ICommandRegistry> registry_;
        std::shared_ptr<Utils::ILogger> logger_;
    };

} // namespace Orcha::Agent::Routes
