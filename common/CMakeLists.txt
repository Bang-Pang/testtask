# interface / header-only library so that server and client can include common headers
add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(common INTERFACE nlohmann_json::nlohmann_json)
