cmake_minimum_required(VERSION 3.16)
project(PythonDownloader)

set(CMAKE_CXX_STANDARD 20)
find_package(cpprestsdk REQUIRED)

add_library(PythonDownloader SHARED PythonDownloader.cpp)
target_include_directories(PythonDownloader PRIVATE ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(PythonDownloader PRIVATE cpprestsdk::cpprest)
set_target_properties(PythonDownloader PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/commands
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/commands
)
