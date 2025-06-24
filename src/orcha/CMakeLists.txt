cmake_minimum_required(VERSION 3.31)
project(orcha LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg integration (if present)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(Boost REQUIRED COMPONENTS thread system)
find_package(cpprestsdk REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

# Core libraries
add_library(core STATIC
        core/ICommand.hpp
        core/CommandRegistry.cpp
        core/CommandRegistry.hpp
)
target_link_libraries(core PUBLIC cpprestsdk::cpprest dl)

# Header-only utils library (modern CMake!)
add_library(utils INTERFACE)
target_include_directories(utils INTERFACE ${CMAKE_SOURCE_DIR}/utils)

add_library(workflow STATIC
        workflow/WorkflowRunner.cpp
        workflow/WorkflowRunner.hpp
)
target_link_libraries(workflow PUBLIC core utils yaml-cpp::yaml-cpp)

add_library(agent STATIC
        agent/CommandAgent.cpp
        agent/CommandAgent.hpp
)
target_link_libraries(agent PUBLIC core workflow Boost::thread Boost::system cpprestsdk::cpprest)

# Main executable
add_executable(orcha
        main.cpp
)
target_link_libraries(orcha PRIVATE agent core workflow yaml-cpp::yaml-cpp utils)

# --- Automatically add all commands as plugins ---
file(GLOB_RECURSE COMMAND_CANDIDATE_DIRS RELATIVE "${CMAKE_SOURCE_DIR}/commands" "${CMAKE_SOURCE_DIR}/commands/*")
foreach(subdir ${COMMAND_CANDIDATE_DIRS})
    if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/commands/${subdir}")
        if(EXISTS "${CMAKE_SOURCE_DIR}/commands/${subdir}/CMakeLists.txt")
            add_subdirectory("commands/${subdir}")
        endif()
    endif()
endforeach()

message(STATUS "Registered command plugins from commands/ directory.")
