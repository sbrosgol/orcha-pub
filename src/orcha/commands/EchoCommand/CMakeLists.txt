cmake_minimum_required(VERSION 3.16)
project(EchoCommand)

set(CMAKE_CXX_STANDARD 20)
find_package(cpprestsdk REQUIRED)

add_library(EchoCommand SHARED EchoCommand.cpp)
target_include_directories(EchoCommand PRIVATE ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(EchoCommand PRIVATE cpprestsdk::cpprest)
set_target_properties(EchoCommand PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/commands
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/commands
)
