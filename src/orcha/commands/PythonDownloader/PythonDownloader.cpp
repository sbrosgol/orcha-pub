#include "../../core/ICommand.hpp"
#include <cpprest/json.h>
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

// Default CPython version downloaded when the caller doesn't pass `version`.
constexpr const char* kDefaultPythonVersion = "3.13.13";

// python-build-standalone publishes redistributable CPython tarballs for
// macOS/Linux under date-stamped GitHub release tags. The TAG appears in both
// the release path and the asset filename, so the default below bundles
// kDefaultPythonVersion. To download a different version, pass `release_tag`
// matching a release that includes it (see
// https://github.com/astral-sh/python-build-standalone/releases).
constexpr const char* kDefaultPbsReleaseTag = "20260510";

std::string make_download_url(const std::string& version, const std::string& release_tag) {
#if defined(_WIN32)
    (void)release_tag;
#if defined(_M_ARM64)
    const std::string arch = "arm64";
#else
    const std::string arch = "amd64";
#endif
    // python.org Windows "embeddable" package.
    return "https://www.python.org/ftp/python/" + version +
           "/python-" + version + "-embed-" + arch + ".zip";
#else
#  if defined(__APPLE__)
#    if defined(__arm64__)
    const std::string triple = "aarch64-apple-darwin";
#    else
    const std::string triple = "x86_64-apple-darwin";
#    endif
#  else
#    if defined(__aarch64__)
    const std::string triple = "aarch64-unknown-linux-gnu";
#    else
    const std::string triple = "x86_64-unknown-linux-gnu";
#    endif
#  endif
    return "https://github.com/astral-sh/python-build-standalone/releases/download/" +
           release_tag + "/cpython-" + version + "+" + release_tag + "-" + triple +
           "-install_only.tar.gz";
#endif
}

const char* archive_name() {
#if defined(_WIN32)
    return "python-embed.zip";
#else
    return "python.tar.gz";
#endif
}

const char* extract_dir_name() {
#if defined(_WIN32)
    return "python_embed";
#else
    return "python";
#endif
}

}  // namespace

class PythonDownloader final : public Orcha::Core::ICommand {
public:
    web::json::value execute(const web::json::value& params) override {
        using namespace web;
        json::value result;

        std::string version = kDefaultPythonVersion;
        std::string release_tag = kDefaultPbsReleaseTag;
        if (params.is_object()) {
            if (params.has_field(U("version"))) {
                version = utility::conversions::to_utf8string(
                    params.at(U("version")).as_string());
            }
            if (params.has_field(U("release_tag"))) {
                release_tag = utility::conversions::to_utf8string(
                    params.at(U("release_tag")).as_string());
            }
        }

        try {
            const std::string url = make_download_url(version, release_tag);
            const std::string archive = archive_name();
            const std::string out_dir = extract_dir_name();

            std::cout << "Downloading embedded Python " << version
                      << " from: " << url << std::endl;

            http::client::http_client client(utility::conversions::to_string_t(url));
            const auto response = client.request(http::methods::GET).get();
            if (response.status_code() / 100 != 2) {
                throw std::runtime_error(
                    "HTTP " + std::to_string(response.status_code()) + " for " + url);
            }

            const concurrency::streams::ostream out =
                concurrency::streams::fstream::open_ostream(
                    utility::conversions::to_string_t(archive)).get();
            (void)response.body().read_to_end(out.streambuf()).wait();
            (void)out.close().wait();
            std::cout << "Download complete: " << archive << std::endl;

            // Fresh extraction directory each run so we don't mix versions.
            std::error_code ec;
            fs::remove_all(out_dir, ec);
            fs::create_directories(out_dir);

#if defined(_WIN32)
            const std::string unzip_cmd =
                "powershell -Command \"Expand-Archive -Path " + archive +
                " -DestinationPath " + out_dir + "\"";
            if (system(unzip_cmd.c_str()) != 0) {
                throw std::runtime_error("Unzip failed.");
            }
            const std::string py_path = out_dir + "\\python.exe";
#else
            const std::string untar_cmd = "tar -xzf " + archive + " -C " + out_dir;
            std::cout << "Extracting archive with: " << untar_cmd << std::endl;
            if (system(untar_cmd.c_str()) != 0) {
                throw std::runtime_error("Untar failed.");
            }

            // python-build-standalone `install_only` tarballs unpack into a
            // top-level `python/` directory with bin/, lib/, include/, etc.
            // Walk the tree to find the python3 binary so we don't hardcode
            // the exact internal layout.
            std::string py_path;
            for (const auto& entry : fs::recursive_directory_iterator(out_dir)) {
                if (!entry.is_regular_file()) continue;
                const auto name = entry.path().filename().string();
                if (name == "python3" || name == "python") {
                    py_path = entry.path().string();
                    break;
                }
            }
            if (py_path.empty()) {
                throw std::runtime_error(
                    "python binary not found in extracted folder: " + out_dir);
            }
            std::cout << "Setting executable permissions on: " << py_path << std::endl;
            if (system(("chmod +x \"" + py_path + "\"").c_str()) != 0) {
                throw std::runtime_error("chmod failed at: " + py_path);
            }
#endif

            std::cout << "Embedded Python is ready at: " << py_path << std::endl;
            result[U("path")] = json::value::string(
                utility::conversions::to_string_t(py_path));
            result[U("version")] = json::value::string(
                utility::conversions::to_string_t(version));
            result[U("success")] = json::value(true);
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << std::endl;
            result[U("success")] = json::value(false);
            result[U("error")] = json::value::string(
                utility::conversions::to_string_t(ex.what()));
        }

        return result;
    }

    [[nodiscard]] std::string name() const override { return "download_python"; }
};

extern "C" Orcha::Core::ICommand* create_command() {
    return new PythonDownloader();
}
