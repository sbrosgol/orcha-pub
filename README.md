Orcha
Orcha is a fast, extensible, and modern command orchestration engine written in C++20.
It dynamically loads command plugins, runs ordered or parallel workflows defined in YAML, supports PowerShell automation, and exposes both CLI and HTTP API.


Features
Plugin-based architecture: Add or update commands without recompiling the core agent

YAML workflow runner: Define multi-step workflows with parameters, piping, and parallelism

Dynamic command loading: Hot-plug commands from the commands/ directory

Fully asynchronous, multi-threaded

Cross-platform: Linux, Windows, macOS

Embedded PowerShell support: Secure, portable scripting even without host PowerShell

REST API: POST workflows or commands to /workflow

Modern C++20, clean architecture

Built with CMake & vcpkg for easy dependency management

Quick Start
1. Clone and Build
bash
Copy
Edit
git clone https://github.com/yourusername/orcha.git
cd orcha
# Install vcpkg dependencies
vcpkg install boost-thread boost-system cpprestsdk yaml-cpp
mkdir build && cd build
cmake ..
make
2. Run HTTP Agent
bash
Copy
Edit
./orcha
# Now listening on http://localhost:8080/
3. Run a YAML Workflow from CLI
bash
Copy
Edit
./orcha ../example_workflow.yaml
Example YAML Workflow
yaml
Copy
Edit
steps:
  - command: add
    params:
      a: 5
      b: 7
  - command: echo
    params:
      message: "The sum is {{step1.output.sum}}"
  - command: pwsh
    params:
      script: |
        "Current user: $env:USER"
Steps run in order by default; use parallel: true for any step to launch it asynchronously.

Reference output of previous steps via {{stepN.output.FIELD}}.

HTTP API Example
bash
Copy
Edit
curl -X POST --data-binary @my_workflow.yaml http://localhost:8080/workflow
Returns a JSON array with per-step output and status.

Adding Commands (Plugins)
Place each new command in its own folder under commands/ (e.g. commands/echo/).

Each must have a CMakeLists.txt to build a shared library.

On restart, Orcha loads all command plugins automatically—no rebuild of Orcha needed.

Sample plugin folder:

bash
Copy
Edit
commands/
  echo/
    EchoCommand.cpp
    EchoCommand.hpp
    CMakeLists.txt
PowerShell Support
The pwsh command plugin will automatically download and embed PowerShell Core (v7+) for safe, isolated PowerShell execution—works even on Linux!

Just pass your script as params.script in YAML or JSON.

Folder Structure
bash
Copy
Edit
orcha/
  src/
    core/            # Core abstractions
    agent/           # HTTP/CLI orchestrator
    utils/           # YAML→JSON, helpers
    workflow/        # YAML workflow engine
  commands/
    echo/
    add/
    pwsh/
  example_workflow.yaml
  CMakeLists.txt
  vcpkg.json
License
MIT (or your chosen license)

Roadmap & Ideas
Hot-reload command plugins (no restart)

Secrets redaction

Native container, SSH, and remote orchestration

Plugin store/registry

About
Orcha is developed by [Your Name/Org].
Inspired by the needs of modern DevOps, security, and infrastructure teams.

