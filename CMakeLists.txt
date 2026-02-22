cmake_minimum_required(VERSION 3.16)
project(integration_system CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

enable_testing()

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(tests)
