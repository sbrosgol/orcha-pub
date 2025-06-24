cmake_minimum_required(VERSION 3.31)
project(PowerShellDownloader)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# vcpkg integration (if present)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(cpprestsdk REQUIRED)


add_library(PowerShellDownloader SHARED PowerShellDownloader.cpp)


target_include_directories(PowerShellDownloader PRIVATE ${CMAKE_SOURCE_DIR}/core ${CMAKE_SOURCE_DIR}/utils)
set_target_properties(PowerShellDownloader PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/commands
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/commands
)

target_link_libraries(PowerShellDownloader PRIVATE cpprestsdk::cpprest)