add_executable(client main.cpp client.cpp client.hpp)
target_link_libraries(client PRIVATE common Threads::Threads)
