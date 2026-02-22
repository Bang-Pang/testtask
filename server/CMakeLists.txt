add_executable(server main.cpp server.cpp server.hpp)
target_link_libraries(server PRIVATE common Threads::Threads)
