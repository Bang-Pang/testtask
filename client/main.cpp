#include "client.hpp"

#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    std::string host    = "127.0.0.1";
    int         port    = 9000;
    int         threads = static_cast<int>(std::thread::hardware_concurrency());
    if (threads < 1) threads = 1;

    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "Usage: client [host] [port] [threads]\n";
        return EXIT_SUCCESS;
    }

    if (argc > 1) host    = argv[1];
    if (argc > 2) port    = std::stoi(argv[2]);
    if (argc > 3) threads = std::stoi(argv[3]);

    try {
        integration::Client cli(host, port, threads);
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << "[client] Fatal: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
