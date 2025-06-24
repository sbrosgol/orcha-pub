# Orcha

Orcha is a fast, extensible, and modern command orchestration engine written in C++20.  
It dynamically loads command plugins, runs ordered or parallel workflows defined in YAML, supports PowerShell automation, and exposes both CLI and HTTP API.

---

## Features

- **Plugin-based architecture:** Add or update commands without recompiling the core agent
- **YAML workflow runner:** Define multi-step workflows with parameters, piping, and parallelism
- **Dynamic command loading:** Hot-plug commands from the `commands/` directory
- **Fully asynchronous, multi-threaded**
- **Cross-platform:** Linux, Windows, macOS
- **Embedded PowerShell support:** Secure, portable scripting even without host PowerShell
- **REST API:** POST workflows or commands to `/workflow`
- **Modern C++20, clean architecture**
- **Built with CMake & vcpkg:** Easy dependency management

---

## Quick Start

### 1. Clone and Build

```bash
git clone https://github.com/yourusername/orcha.git
cd orcha
# Install vcpkg dependencies
vcpkg install boost-thread boost-system cpprestsdk yaml-cpp
mkdir build && cd build
cmake ..
make
