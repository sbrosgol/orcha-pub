#include "../../core/ICommand.hpp"
#include <cpprest/json.h>
#include <string>
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

#if defined(_WIN32)
const std::string ps_url = "https://github.com/PowerShell/PowerShell/releases/download/v7.4.2/PowerShell-7.4.2-win-x64.zip";
const std::string ps_archive = "PowerShell.zip";
const std::string ps_folder = "pwsh_win";
#else
#if defined(__APPLE__)
const std::string ps_url = "https://github.com/PowerShell/PowerShell/releases/download/v7.4.2/powershell-7.4.2-osx-x64.tar.gz";
#else
const std::string ps_url = "https://github.com/PowerShell/PowerShell/releases/download/v7.4.2/powershell-7.4.2-linux-x64.tar.gz";
#endif
const std::string ps_archive = "powershell.tar.gz";
const std::string ps_folder = "pwsh";
#endif

class PowerShellDownloader : public ICommand {
public:
    web::json::value execute(const web::json::value& params) override {
        using namespace web;
        using namespace utility;
        json::value result;

        try {
            // Create client and GET request
            web::http::client::http_client client(utility::conversions::to_string_t(ps_url));
            std::cout << "Downloading PowerShell Core from: " << ps_url << std::endl;
            auto response = client.request(web::http::methods::GET).get();

            // Write to file using cpprestsdk streams
            concurrency::streams::ostream out = concurrency::streams::fstream::open_ostream(ps_archive).get();
            response.body().read_to_end(out.streambuf()).wait();
            (void)out.close().wait();

            std::cout << "Download complete: " << ps_archive << std::endl;

            // Extract and set executable
#if defined(_WIN32)
            std::string unzip_cmd = "powershell -Command \"Expand-Archive -Path " + ps_archive + " -DestinationPath " + ps_folder + "\"";
            int unzip_result = system(unzip_cmd.c_str());
            if (unzip_result != 0) throw std::runtime_error("Unzip failed.");
            std::string ps_path = ps_folder + "\\pwsh.exe";
#else
            std::string mkdir_cmd = "mkdir -p " + ps_folder;
            system(mkdir_cmd.c_str());
            std::string untar_cmd = "tar -xzf " + ps_archive + " -C " + ps_folder + " --strip-components=1";
            int untar_result = system(untar_cmd.c_str());
            if (untar_result != 0) throw std::runtime_error("Untar failed.");
            std::string ps_path = ps_folder + "/pwsh";
            int chmod_result = system(("chmod +x " + ps_path).c_str());
            if (chmod_result != 0) throw std::runtime_error("chmod failed.");
#endif
            std::cout << "PowerShell Core is ready at: " << ps_path << std::endl;

            result[U("path")] = json::value::string(conversions::to_string_t(ps_path));
            result[U("success")] = json::value(true);
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << std::endl;
            result[U("success")] = json::value(false);
            result[U("error")] = json::value::string(ex.what());
        }
        return result;
    }

    std::string name() const override { return "download_pwsh"; }
};

extern "C" ICommand* create_command() {
    return new PowerShellDownloader();
}
